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

#include "src/core/audio.h"

#include "dbus_connection_handle.h"
#include "dbus_event_loop.h"

namespace repowerd
{
    class Log;

    class GemianAudio : public Audio
    {
    public:
        GemianAudio(std::shared_ptr<Log> const& log,
                std::string const& dbus_bus_address);

        void start_processing() override;

        HandlerRegistration register_audio_headphone_cs_handler(
                AudioHeadphoneCSHandler const& handler) override;
        HandlerRegistration register_audio_keep_alive_handler(
                AudioKeepAliveHandler const& handler) override;

    private:
        void dbus_method_call(
                GDBusConnection* connection,
                gchar const* sender,
                gchar const* object_path,
                gchar const* interface_name,
                gchar const* method_name,
                GVariant* parameters,
                GDBusMethodInvocation* invocation);

        void dbus_unknown_method(
                std::string const& sender, std::string const& name);

        void dbus_SetAudioHeadphoneCS(
                std::string const& sender,
                std::string const& speaker);

        void dbus_SetAudioKeepAlive(
                std::string const& sender,
                bool keep_alive);

        std::shared_ptr<Log> const log;
        DBusConnectionHandle dbus_connection;
        DBusEventLoop dbus_event_loop;
        HandlerRegistration audio_handler_registration;

        AudioHeadphoneCSHandler audio_headphone_cs_handler;
        AudioKeepAliveHandler audio_keep_alive_handler;
    };

}
