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

#include "src/core/audio_headphone_cs.h"

#include "dbus_connection_handle.h"
#include "dbus_event_loop.h"

namespace repowerd
{

    class GemianAudioHeadphoneCS : public AudioHeadphoneCS
    {
    public:
        GemianAudioHeadphoneCS(std::string const& dbus_bus_address);

        void start_processing() override;

        HandlerRegistration register_audio_headphone_cs_handler(
                AudioHeadphoneCSHandler const& handler) override;

    private:
        void handle_dbus_signal(
                GDBusConnection* connection,
                gchar const* sender,
                gchar const* object_path,
                gchar const* interface_name,
                gchar const* signal_name,
                GVariant* parameters);

        DBusConnectionHandle dbus_connection;
        DBusEventLoop dbus_event_loop;
        HandlerRegistration dbus_signal_handler_registration;

        AudioHeadphoneCSHandler audio_headphone_cs_handler;

        HandlerRegistration headphone_handler_registration;
    };

}
