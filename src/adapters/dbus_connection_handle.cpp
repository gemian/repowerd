/*
 * Copyright © 2016 Canonical Ltd.
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
 */

#include "dbus_connection_handle.h"
#include "scoped_g_error.h"

#include <stdexcept>

repowerd::DBusConnectionHandle::DBusConnectionHandle(std::string const& address)
{
    repowerd::ScopedGError error;
    bool auth_client = false;
    if (address.find("/user/") == std::string::npos) {
        auth_client = true;
    }
    auto flags = GDBusConnectionFlags(
            (auth_client ? G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT|G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION : G_DBUS_CONNECTION_FLAGS_NONE)
    );

    connection = g_dbus_connection_new_for_address_sync(
        address.c_str(),
        flags,
        nullptr,
        nullptr,
        error);

    if (!connection)
    {
        throw std::runtime_error(
            "Failed to connect to DBus bus with address '" +
                address + "': " + error.message_str());
    }
}

repowerd::DBusConnectionHandle::~DBusConnectionHandle()
{
    g_dbus_connection_close_sync(connection, nullptr, nullptr);
}

void repowerd::DBusConnectionHandle::request_name(char const* name) const
{
    static constexpr uint32_t DBUS_NAME_FLAG_DO_NOT_QUEUE = 0x4;
    static constexpr uint32_t DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER = 0x1;

    repowerd::ScopedGError error;
    int const timeout_default = -1;
    auto const null_cancellable = nullptr;

    auto result = g_dbus_connection_call_sync(
        connection,
        "org.freedesktop.DBus",
        "/org/freedesktop/DBus",
        "org.freedesktop.DBus",
        "RequestName",
        g_variant_new("(su)", name, DBUS_NAME_FLAG_DO_NOT_QUEUE),
        G_VARIANT_TYPE("(u)"),
        G_DBUS_CALL_FLAGS_NONE,
        timeout_default,
        null_cancellable,
        error);

    if (!result)
    {
        throw std::runtime_error(
            "Failed to request DBus name '" +
                std::string{name} + "': " + error.message_str());
    }

    uint32_t request_name_reply{0};

    g_variant_get(result, "(u)", &request_name_reply);
    g_variant_unref(result);

    if (request_name_reply != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
    {
        throw std::runtime_error(
            "Failed to become the primary owner of DBus name '" +
                std::string{name} + "'");
    }
}

repowerd::DBusConnectionHandle::operator GDBusConnection*() const
{
    return connection;
}
