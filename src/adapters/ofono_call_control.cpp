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

#include "ofono_call_control.h"

#include "src/core/log.h"
#include <algorithm>

namespace {
    char const *const log_tag = "OfonoCallControl";
}

repowerd::OfonoCallControl::OfonoCallControl(
        std::shared_ptr<Log> const &log,
        std::string const &dbus_bus_address)
        : log{log},
          dbus_connection{dbus_bus_address} {
}

void repowerd::OfonoCallControl::hang_up_and_accept_call() {
    log->log(log_tag, "hang up and accept call");

    //This needs to happen on the session bus, for now gemian-lock does this directly
}
