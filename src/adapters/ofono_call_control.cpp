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
    char const *const dbus_telephony_approver_name = "com.canonical.Approver";
    char const *const dbus_telephony_approver_path = "/com/canonical/Approver";
    char const *const dbus_telephony_approver_interface = "com.canonical.TelephonyServiceApprover";
    char const *const dbus_telephony_approver_message = "HangUpAndAcceptCall";
}

repowerd::OfonoCallControl::OfonoCallControl(
        std::shared_ptr<Log> const &log,
        std::string const &dbus_bus_address)
        : log{log},
          dbus_connection{dbus_bus_address} {
}

void repowerd::OfonoCallControl::hang_up_and_accept_call() {

    log->log(log_tag, "hang up and accept call");

    auto m = g_dbus_message_new_method_call(dbus_telephony_approver_name,
                                            dbus_telephony_approver_path,
                                            dbus_telephony_approver_interface,
                                            dbus_telephony_approver_message);
    g_dbus_connection_send_message(dbus_connection, m, G_DBUS_SEND_MESSAGE_FLAGS_NONE, nullptr, nullptr);
    g_object_unref(m);
}
