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

#include "gemian_audio.h"
#include "event_loop_handler_registration.h"

#include "src/core/log.h"

namespace
{
    auto const null_headphone_cs_handler = [](repowerd::AudioHeadphoneCSState){};
    auto const null_keep_alive_handler = [](repowerd::AudioKeepAliveState){};
    char const* const dbus_audio_name = "org.thinkglobally.Gemian.Audio";
    char const* const dbus_audio_path = "/org/thinkglobally/Gemian/Audio";
    char const* const dbus_audio_interface = "org.thinkglobally.Gemian.Audio";

    char const* const log_tag = "Audio";

    char const* const dbus_audio_introspection = R"(
<node>
  <interface name='org.thinkglobally.Gemian.Audio'>
    <method name='SetAudioHeadphoneCS'>
      <arg type='s' name='speaker' direction='in' />
    </method>
    <method name='SetAudioKeepAlive'>
      <arg type='b' name='active' direction='in' />
    </method>
  </interface>
</node>)";
}

repowerd::GemianAudio::GemianAudio(std::shared_ptr<Log> const& log,
        std::string const& dbus_bus_address)
        : log{log},
          dbus_connection{dbus_bus_address},
          dbus_event_loop{"Audio"},
          audio_headphone_cs_handler{null_headphone_cs_handler},
          audio_keep_alive_handler{null_keep_alive_handler}
{
}

void repowerd::GemianAudio::start_processing()
{
    log->log(log_tag, "start_processing");

    audio_handler_registration = dbus_event_loop.register_object_handler(
            dbus_connection,
            dbus_audio_path,
            dbus_audio_introspection,
            [this] (GDBusConnection* connection,
                    gchar const* sender,
                    gchar const* object_path,
                    gchar const* interface_name,
                    gchar const* method_name,
                    GVariant* parameters,
                    GDBusMethodInvocation* invocation)
            {
                try
                {
                    dbus_method_call(
                            connection, sender, object_path, interface_name,
                            method_name, parameters, invocation);
                }
                catch (std::invalid_argument const& e)
                {
                    g_dbus_method_invocation_return_error_literal(
                            invocation, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS, e.what());
                }
                catch (std::exception const& e)
                {
                    g_dbus_method_invocation_return_error_literal(
                            invocation, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, e.what());
                }
            });

    dbus_connection.request_name(dbus_audio_name);
}

void repowerd::GemianAudio::dbus_method_call(
        GDBusConnection* /*connection*/,
        gchar const* sender_cstr,
        gchar const* /*object_path_cstr*/,
        gchar const* /*interface_name_cstr*/,
        gchar const* method_name_cstr,
        GVariant* parameters,
        GDBusMethodInvocation* invocation)
{
    std::string const sender{sender_cstr ? sender_cstr : ""};
    std::string const method_name{method_name_cstr ? method_name_cstr : ""};

    log->log(log_tag, "dbus_method_call(%s,%s)", sender_cstr, method_name_cstr);

    if (method_name == "SetAudioHeadphoneCS")
    {
        char const* headphone_speaker{""};
        g_variant_get(parameters, "(&s)", &headphone_speaker);

        dbus_SetAudioHeadphoneCS(sender, headphone_speaker);

        g_dbus_method_invocation_return_value(invocation, nullptr);
    }
    else if (method_name == "SetAudioKeepAlive")
    {
        gboolean keep_alive{false};
        g_variant_get(parameters, "(b)", &keep_alive);

        dbus_SetAudioKeepAlive(sender, keep_alive);

        g_dbus_method_invocation_return_value(invocation, nullptr);
    }
    else
    {
        dbus_unknown_method(sender, method_name);

        g_dbus_method_invocation_return_error_literal(
                invocation, G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED, "");
    }
}

void repowerd::GemianAudio::dbus_unknown_method(
        std::string const& sender, std::string const& name)
{
    log->log(log_tag, "dbus_unknown_method(%s,%s)", sender.c_str(), name.c_str());
}

repowerd::HandlerRegistration repowerd::GemianAudio::register_audio_headphone_cs_handler(
        const repowerd::AudioHeadphoneCSHandler &handler)
{
    return EventLoopHandlerRegistration{
            dbus_event_loop,
            [this, &handler] { this->audio_headphone_cs_handler = handler; },
            [this] { this->audio_headphone_cs_handler = null_headphone_cs_handler; }};
}

repowerd::HandlerRegistration repowerd::GemianAudio::register_audio_keep_alive_handler(
        const repowerd::AudioKeepAliveHandler &handler)
{
    return EventLoopHandlerRegistration{
            dbus_event_loop,
            [this, &handler] { this->audio_keep_alive_handler = handler; },
            [this] { this->audio_keep_alive_handler = null_keep_alive_handler; }};
}

void repowerd::GemianAudio::dbus_SetAudioHeadphoneCS(
        std::string const& /*sender*/,
        std::string const& speaker)
{
    if (speaker == "Left") {
        audio_headphone_cs_handler(repowerd::AudioHeadphoneCSState::left);
    } else if (speaker == "Right") {
        audio_headphone_cs_handler(repowerd::AudioHeadphoneCSState::right);
    }
}

void repowerd::GemianAudio::dbus_SetAudioKeepAlive(
        std::string const& /*sender*/,
        bool keep_alive)
{
    if (keep_alive) {
        audio_keep_alive_handler(repowerd::AudioKeepAliveState::active);
    } else {
        audio_keep_alive_handler(repowerd::AudioKeepAliveState::idle);
    }
}
