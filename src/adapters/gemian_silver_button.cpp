/*
 * Copyright © 2019 Gemian
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
 * Authored by: Adam Boardman
 */

#include "gemian_silver_button.h"
#include "event_loop_handler_registration.h"

namespace
{
    auto const null_handler = [](repowerd::SilverButtonState){};
    char const* const dbus_silver_button_name = "org.thinkglobally.Gemian.SilverButton";
    char const* const dbus_silver_button_path = "/org/thinkglobally/Gemian/SilverButton";
    char const* const dbus_silver_button_interface = "org.thinkglobally.Gemian.SilverButton";
}

repowerd::GemianSilverButton::GemianSilverButton(
        std::string const& dbus_bus_address)
        : dbus_connection{dbus_bus_address},
          dbus_event_loop{"SilverButton"},
          silver_button_handler{null_handler}
{
}

void repowerd::GemianSilverButton::start_processing()
{
    dbus_signal_handler_registration = dbus_event_loop.register_signal_handler(
            dbus_connection,
            dbus_silver_button_name,
            dbus_silver_button_interface,
            nullptr,
            dbus_silver_button_path,
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
}

repowerd::HandlerRegistration repowerd::GemianSilverButton::register_silver_button_handler(
        SilverButtonHandler const& handler)
{
    return EventLoopHandlerRegistration{
            dbus_event_loop,
            [this, &handler] { this->silver_button_handler = handler; },
            [this] { this->silver_button_handler = null_handler; }};
}

void repowerd::GemianSilverButton::handle_dbus_signal(
        GDBusConnection* /*connection*/,
        gchar const* /*sender*/,
        gchar const* /*object_path*/,
        gchar const* /*interface_name*/,
        gchar const* signal_name_cstr,
        GVariant* /*parameters*/)
{
    std::string const signal_name{signal_name_cstr ? signal_name_cstr : ""};

    if (signal_name == "OnPress")
        silver_button_handler(repowerd::SilverButtonState::onPressed);
    else if (signal_name == "OnRelease")
        silver_button_handler(repowerd::SilverButtonState::released);
}
