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

#include "handler_registration.h"

#include <functional>

namespace repowerd
{
    enum class AudioKeepAliveState{idle, active};
    enum class AudioHeadphoneCSState{left, right};
    using AudioKeepAliveHandler = std::function<void(AudioKeepAliveState)>;
    using AudioHeadphoneCSHandler = std::function<void(AudioHeadphoneCSState)>;

    class Audio
    {
    public:
        virtual ~Audio() = default;

        virtual void start_processing() = 0;

        virtual HandlerRegistration register_audio_keep_alive_handler(
                AudioKeepAliveHandler const& handler) = 0;
        virtual HandlerRegistration register_audio_headphone_cs_handler(
                AudioHeadphoneCSHandler const& handler) = 0;

    protected:
        Audio() = default;
        Audio (Audio const&) = default;
        Audio& operator=(Audio const&) = default;
    };

}
