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

#include "src/core/session_bus_provider.h"
#include "src/core/lock.h"
#include "src/core/log.h"

#include "dbus_connection_handle.h"
#include "dbus_event_loop.h"

namespace repowerd
{

    class X11Lock : public Lock
    {
    public:
        X11Lock(std::shared_ptr<Log> const& log,
                std::shared_ptr<SessionBusProvider> const& dbus_session_bus_provider);

        void start_processing() override;
        HandlerRegistration register_lock_handler(
            LockHandler const& handler) override;

    private:
        void handle_dbus_signal(
                GDBusConnection* connection,
                gchar const* sender,
                gchar const* object_path,
                gchar const* interface_name,
                gchar const* signal_name,
                GVariant* parameters);

        std::shared_ptr<Log> const log;
        std::shared_ptr<SessionBusProvider> dbus_session_bus_provider;
        DBusEventLoop dbus_event_loop;
        HandlerRegistration dbus_signal_handler_registration;

        LockHandler lock_handler;
    };

}
