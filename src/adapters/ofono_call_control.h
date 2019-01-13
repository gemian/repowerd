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

#pragma once

#include "src/core/call_control.h"
#include "dbus_connection_handle.h"
#include "dbus_event_loop.h"
#include "fd.h"
#include "event_loop.h"

#include <memory>

namespace repowerd
{

    class Log;

    class OfonoCallControl : public CallControl
    {
    public:
        OfonoCallControl(
                std::shared_ptr<Log> const& log,
                std::string const& dbus_bus_address);

        void hang_up_and_accept_call() override;

    private:
        std::shared_ptr<Log> const log;
        DBusConnectionHandle dbus_connection;
    };

}
