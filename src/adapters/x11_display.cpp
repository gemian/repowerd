/*
 * Copyright © 2016 Canonical Ltd.
 * Copyright © 2018 Gemian
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 * Modified by: Adam Boardman <adamboardman@gmail.com>
 */

#include "x11_display.h"
#include "scoped_g_error.h"
#include "x11_lock.h"

#include <gio/gio.h>
#include <pwd.h>

namespace
{
    char const* const dbus_logind_name = "org.freedesktop.login1";
    char const* const dbus_seat_path = "/org/freedesktop/login1/seat/seat0";
    char const* const dbus_seat_interface = "org.freedesktop.login1.Seat";
    char const* const dbus_session_interface = "org.freedesktop.login1.Session";

//Swap this to whatever signals can be monitored for external HDMI monitor availability
    char const* const x11_display_bus_name = "org.thinkglobally.Gemian.Display";
    char const* const x11_display_object_path = "/org/thinkglobally/Gemian/Display";
    char const* const x11_display_interface_name = "org.thinkglobally.Gemian.Display";
    char const* const log_tag = "X11Display";

    std::string filter_to_str(repowerd::DisplayPowerControlFilter filter)
    {
        std::string filter_str;

        if (filter == repowerd::DisplayPowerControlFilter::all)
            filter_str = "all";
        else if (filter == repowerd::DisplayPowerControlFilter::internal)
            filter_str = "internal";
        else if (filter == repowerd::DisplayPowerControlFilter::external)
            filter_str = "external";
        else
            filter_str = "(unknown)";

        return filter_str;
    }

}


repowerd::X11Display::X11Display(
        std::shared_ptr<Log> const& log,
        std::shared_ptr<Exec> const& exec,
        std::shared_ptr<Lock> const& lock,
        std::string const& dbus_bus_address,
		std::shared_ptr<SessionBusProvider> const& dbus_session_bus_provider)
        : log{log},
          exec{exec},
          lock{lock},
          dbus_connection{dbus_bus_address},
          dbus_session_bus_provider{dbus_session_bus_provider},
          dbus_event_loop{"Display"},
          has_active_external_displays_{false},
          active_username_{}
{

    dbus_signal_handler_registration = dbus_event_loop.register_signal_handler(
            dbus_connection,
            x11_display_bus_name,
            "org.freedesktop.DBus.Properties",
            "PropertiesChanged",
            x11_display_object_path,
            [this] (
                    GDBusConnection* connection,
                    gchar const* sender,
                    gchar const* object_path,
                    gchar const* interface_name,
                    gchar const* signal_name,
                    GVariant* parameters)
            {
                handle_dbus_signal(
                        connection, sender, object_path, interface_name,
                        signal_name, parameters);
            });

    dbus_event_loop.enqueue([this] { dbus_query_active_session(); }).get();
    dbus_event_loop.enqueue([this] { dbus_query_active_outputs(); }).get();
}

void repowerd::X11Display::turn_on(DisplayPowerControlFilter filter)
{
    auto const filter_str = filter_to_str(filter);

    log->log(log_tag, "turn_on(%s)", filter_str.c_str());

    std::string on_cmd = std::string("/bin/su - ")+active_username_+" -c \"DISPLAY=:0 xrandr --output hwcomposer --auto; DISPLAY=:0 xset dpms force on\"";
    int ret = exec->exec(on_cmd.c_str());

    log->log(log_tag, "turned_on(%s) - %s(%d)", filter_str.c_str(), on_cmd.c_str(), ret);
}

void repowerd::X11Display::turn_off(DisplayPowerControlFilter filter, bool lid_closed)
{
    auto const filter_str = filter_to_str(filter);
    DBusConnectionHandle dbus_session_connection{dbus_session_bus_provider->Get()};

    g_dbus_connection_call(
            dbus_session_connection,
            "org.freedesktop.ScreenSaver",
            "/org/freedesktop/ScreenSaver",
            "org.freedesktop.ScreenSaver",
            "Lock",
            nullptr,
            nullptr,
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            nullptr,
            nullptr,
            nullptr);

    log->log(log_tag, "turn_off(%s)", filter_str.c_str());

    std::string off_cmd = std::string("/bin/su - ") + active_username_;
    if (lid_closed) {
        off_cmd += " -c \"DISPLAY=:0 xrandr --output hwcomposer --off; DISPLAY=:0 xset dpms force off\"";
    } else {
        off_cmd += " -c \"DISPLAY=:0 xset dpms force off\"";
    }
    int ret = exec->exec(off_cmd.c_str());

    log->log(log_tag, "turned_off(%s) - %s(%d)", filter_str.c_str(), off_cmd.c_str(), ret);
}

bool repowerd::X11Display::has_active_external_displays()
{
    return has_active_external_displays_;
}

std::string repowerd::X11Display::active_username()
{
    return active_username_;
}

void repowerd::X11Display::handle_dbus_signal(
        GDBusConnection* /*connection*/,
        gchar const* /*sender*/,
        gchar const* /*object_path*/,
        gchar const* /*interface_name*/,
        gchar const* signal_name_cstr,
        GVariant* parameters)
{
    std::string const signal_name{signal_name_cstr ? signal_name_cstr : ""};

    if (signal_name == "PropertiesChanged")
    {
        char const* properties_interface_cstr{""};
        GVariantIter* properties_iter{nullptr};
        g_variant_get(parameters, "(&sa{sv}as)",
                      &properties_interface_cstr, &properties_iter, nullptr);

        std::string const properties_interface{properties_interface_cstr};

        if (properties_interface == x11_display_interface_name)
            dbus_PropertiesChanged(properties_iter);

        g_variant_iter_free(properties_iter);
    }
}

void repowerd::X11Display::dbus_PropertiesChanged(
        GVariantIter* properties_iter)
{
    char const* key_cstr{""};
    GVariant* value{nullptr};

    while (g_variant_iter_next(properties_iter, "{&sv}", &key_cstr, &value))
    {
        auto const key_str = std::string{key_cstr ? key_cstr : ""};

        if (key_str == "ActiveOutputs")
        {
            gint32 internal{0};
            gint32 external{0};

            g_variant_get(value, "(ii)", &internal, &external);

            dbus_ActiveOutputs(internal, external);
        }

        g_variant_unref(value);
    }
}

void repowerd::X11Display::dbus_ActiveOutputs(
        int32_t internal, int32_t external)
{
    log->log(log_tag, "dbus_ActiveOutputs(internal=%d, external=%d)", internal, external);

    has_active_external_displays_ = (external > 0);
}

void repowerd::X11Display::dbus_query_active_outputs()
{
    log->log(log_tag, "dbus_query_active_outputs()");

    int constexpr timeout_default = -1;
    auto constexpr null_cancellable = nullptr;
    ScopedGError error;

    auto const result = g_dbus_connection_call_sync(
            dbus_connection,
            x11_display_bus_name,
            x11_display_object_path,
            "org.freedesktop.DBus.Properties",
            "Get",
            g_variant_new("(ss)", x11_display_interface_name, "ActiveOutputs"),
            G_VARIANT_TYPE("(v)"),
            G_DBUS_CALL_FLAGS_NONE,
            timeout_default,
            null_cancellable,
            error);

    if (!result)
    {
        log->log(log_tag, "dbus_get_active_outputs() failed to get ActiveOutputs: %s",
                 error.message_str().c_str());
        return;
    }

    GVariant* property_variant{nullptr};
    g_variant_get(result, "(v)", &property_variant);

    gint32 internal{0};
    gint32 external{0};

    g_variant_get(property_variant, "(ii)", &internal, &external);

    dbus_ActiveOutputs(internal, external);

    g_variant_unref(property_variant);
    g_variant_unref(result);
}

void repowerd::X11Display::dbus_query_active_session()
{
    auto const active_session = dbus_get_active_session();
    if (!active_session.first.empty())
    {
        active_username_ = dbus_get_session_user_name(active_session.second);
        set_active_uid(dbus_get_session_user_id(active_session.second));
        lock->start_processing();
    }
}

std::pair<std::string,std::string> repowerd::X11Display::dbus_get_active_session()
{
    int constexpr timeout_default = -1;
    auto constexpr null_cancellable = nullptr;
    ScopedGError error;

    auto const result = g_dbus_connection_call_sync(
            dbus_connection,
            dbus_logind_name,
            dbus_seat_path,
            "org.freedesktop.DBus.Properties",
            "Get",
            g_variant_new("(ss)", dbus_seat_interface, "ActiveSession"),
            G_VARIANT_TYPE("(v)"),
            G_DBUS_CALL_FLAGS_NONE,
            timeout_default,
            null_cancellable,
            error);

    if (!result)
    {
        log->log(log_tag, "dbus_get_active_session() failed to get ActiveSession: %s",
                 error.message_str().c_str());
        return {"", ""};
    }

    GVariant* active_session_variant{nullptr};
    g_variant_get(result, "(v)", &active_session_variant);

    char const* session_id_cstr{""};
    char const* session_path_cstr{""};
    g_variant_get(active_session_variant, "(&s&o)", &session_id_cstr, &session_path_cstr);

    std::string const session_id{session_id_cstr};
    std::string const session_path{session_path_cstr};

    g_variant_unref(active_session_variant);
    g_variant_unref(result);

    return {session_id, session_path};
}

std::string repowerd::X11Display::dbus_get_session_user_name(std::string const& session_path)
{
    int constexpr timeout_default = -1;
    auto constexpr null_cancellable = nullptr;
    ScopedGError error;

    auto const result = g_dbus_connection_call_sync(
            dbus_connection,
            dbus_logind_name,
            session_path.c_str(),
            "org.freedesktop.DBus.Properties",
            "Get",
            g_variant_new("(ss)", dbus_session_interface, "Name"),
            G_VARIANT_TYPE("(v)"),
            G_DBUS_CALL_FLAGS_NONE,
            timeout_default,
            null_cancellable,
            error);

    if (!result)
    {
        log->log(log_tag, "dbus_get_session_type() failed to get session Name: %s",
                 error.message_str().c_str());
        return "";
    }

    GVariant* name_variant{nullptr};
    g_variant_get(result, "(v)", &name_variant);

    char const* session_name_cstr{""};
    g_variant_get(name_variant, "&s", &session_name_cstr);

    std::string const session_name{session_name_cstr};

    g_variant_unref(name_variant);
    g_variant_unref(result);

    return session_name;
}

guint32 repowerd::X11Display::dbus_get_session_user_id(std::string const& session_path)
{
    int constexpr timeout_default = -1;
    auto constexpr null_cancellable = nullptr;
    ScopedGError error;

    auto const result = g_dbus_connection_call_sync(
            dbus_connection,
            dbus_logind_name,
            session_path.c_str(),
            "org.freedesktop.DBus.Properties",
            "Get",
            g_variant_new("(ss)", dbus_session_interface, "User"),
            G_VARIANT_TYPE("(v)"),
            G_DBUS_CALL_FLAGS_NONE,
            timeout_default,
            null_cancellable,
            error);

    if (!result)
    {
        log->log(log_tag, "dbus_get_session_type() failed to get session Name: %s",
                 error.message_str().c_str());
        return 0;
    }

    GVariant *user_variant{nullptr};
    g_variant_get(result, "(v)", &user_variant);

    guint32 user_id{0};
    char const* user_path{""};
    g_variant_get(user_variant, "(uo)", &user_id, &user_path);
    log->log(log_tag, "User Id: %d, User Path: %s",user_id, user_path);

    g_variant_unref(user_variant);
    g_variant_unref(result);

    return user_id;
}

void repowerd::X11Display::set_active_username(const char *string)
{
    active_username_ = string;
}

void repowerd::X11Display::set_active_uid(const guint32 uid)
{
    dbus_session_bus_provider->UpdateSessionBus(uid);
}
