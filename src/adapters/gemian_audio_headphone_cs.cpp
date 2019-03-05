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

#include "gemian_audio_headphone_cs.h"
#include "event_loop_handler_registration.h"

namespace
{
    auto const null_handler = [](repowerd::AudioHeadphoneCSState){};
    char const* const dbus_audio_headphone_cs_name = "org.thinkglobally.Gemian.Audio.HeadphoneCS";
    char const* const dbus_audio_headphone_cs_path = "/org/thinkglobally/Gemian/Audio.HeadphoneCS";
    char const* const dbus_audio_headphone_cs_interface = "org.thinkglobally.Gemian.Audio.HeadphoneCS";
}

repowerd::GemianAudioHeadphoneCS::GemianAudioHeadphoneCS(std::string const& dbus_bus_address)
        : dbus_connection{dbus_bus_address},
          dbus_event_loop{"AudioHeadphoneCS"},
          audio_headphone_cs_handler{null_handler}
{
}

void repowerd::GemianAudioHeadphoneCS::start_processing()
{
    dbus_signal_handler_registration = dbus_event_loop.register_signal_handler(
            dbus_connection,
            dbus_audio_headphone_cs_name,
            dbus_audio_headphone_cs_interface,
            nullptr,
            dbus_audio_headphone_cs_path,
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

repowerd::HandlerRegistration repowerd::GemianAudioHeadphoneCS::register_audio_headphone_cs_handler(
        const repowerd::AudioHeadphoneCSHandler &handler) {
    return EventLoopHandlerRegistration{
            dbus_event_loop,
            [this, &handler] { this->audio_headphone_cs_handler = handler; },
            [this] { this->audio_headphone_cs_handler = null_handler; }};
}

void repowerd::GemianAudioHeadphoneCS::handle_dbus_signal(
        GDBusConnection* /*connection*/,
        gchar const* /*sender*/,
        gchar const* /*object_path*/,
        gchar const* /*interface_name*/,
        gchar const* signal_name_cstr,
        GVariant* /*parameters*/)
{
    std::string const signal_name{signal_name_cstr ? signal_name_cstr : ""};

    if (signal_name == "Left")
        audio_headphone_cs_handler(repowerd::AudioHeadphoneCSState::left);
    else if (signal_name == "Right")
        audio_headphone_cs_handler(repowerd::AudioHeadphoneCSState::right);
}
