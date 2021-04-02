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

#pragma once

#include "src/core/display_power_control.h"
#include "src/core/display_information.h"
#include "src/core/handler_registration.h"
#include "src/core/session_bus_provider.h"
#include "src/core/log.h"
#include "src/core/exec.h"
#include "src/core/lock.h"

#include "dbus_connection_handle.h"
#include "dbus_event_loop.h"

#include <memory>
#include <atomic>

namespace repowerd
{
    class X11Display : public DisplayPowerControl, public DisplayInformation
    {
    public:
        X11Display(
                std::shared_ptr<Log> const& log,
                std::shared_ptr<Exec> const& exec,
                std::shared_ptr<Lock> const& lock,
                std::string const& dbus_bus_address,
                std::shared_ptr<SessionBusProvider> const& dbus_session_bus_provider);

        // From DisplayPowerControl
        void turn_on(DisplayPowerControlFilter filter) override;
        void turn_off(DisplayPowerControlFilter filter, bool lid_closed) override;

        // From DisplayInformation
        bool has_active_external_displays() override;

        // For testing
        std::string active_username();
        void set_active_username(const char *string);
        void set_active_uid(guint32 uid);

    private:
        void handle_dbus_signal(
                GDBusConnection* connection,
                gchar const* sender,
                gchar const* object_path,
                gchar const* interface_name,
                gchar const* signal_name,
                GVariant* parameters);
        void dbus_query_active_outputs();
        void dbus_PropertiesChanged(GVariantIter* properties_iter);
        void dbus_ActiveOutputs(int32_t internal, int32_t external);

        std::shared_ptr<Log> const log;
        std::shared_ptr<Exec> const exec;
        std::shared_ptr<Lock> const lock;
        DBusConnectionHandle dbus_connection;
        std::shared_ptr<SessionBusProvider> dbus_session_bus_provider;
        DBusEventLoop dbus_event_loop;
        HandlerRegistration dbus_signal_handler_registration;
        std::atomic<bool> has_active_external_displays_;
        std::string active_username_;

        std::pair<std::string, std::string> dbus_get_active_session();

        std::string dbus_get_session_user_name(const std::string &session_path);
        guint32 dbus_get_session_user_id(std::string const& session_path);

        void dbus_query_active_session();
    };

}
