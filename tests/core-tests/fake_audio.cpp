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

#include "fake_audio.h"

namespace rt = repowerd::test;

void rt::FakeAudio::start_processing()
{
    mock.start_processing();
}

repowerd::HandlerRegistration rt::FakeAudio::register_audio_headphone_cs_handler(
        const repowerd::AudioHeadphoneCSHandler &handler)
{
    mock.register_audio_headphone_cs_handler(handler);
    this->headphone_cs_handler = handler;
    return HandlerRegistration{
            [this]
            {
                mock.unregister_audio_headphone_cs_handler();
                this->headphone_cs_handler = [](AudioHeadphoneCSState){};
            }};
}

repowerd::HandlerRegistration rt::FakeAudio::register_audio_keep_alive_handler(
        const repowerd::AudioKeepAliveHandler &handler)
{
    mock.register_audio_keep_alive_handler(handler);
    this->keep_alive_handler = handler;
    return HandlerRegistration{
            [this]
            {
                mock.unregister_audio_keep_alive_handler();
                this->keep_alive_handler = [](AudioKeepAliveState){};
            }};
}

void rt::FakeAudio::leftUp()
{
    headphone_cs_handler(AudioHeadphoneCSState::left);
}

void rt::FakeAudio::rightUp()
{
    headphone_cs_handler(AudioHeadphoneCSState::right);
}

void rt::FakeAudio::idle()
{
    keep_alive_handler(AudioKeepAliveState::idle);
}

void rt::FakeAudio::active()
{
    keep_alive_handler(AudioKeepAliveState::active);
}