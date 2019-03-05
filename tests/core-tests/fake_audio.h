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

#include <gmock/gmock.h>

namespace repowerd
{
    namespace test
    {

        class FakeAudio : public Audio
        {
        public:
            void start_processing() override;

            HandlerRegistration register_audio_headphone_cs_handler(AudioHeadphoneCSHandler const& handler) override;
            HandlerRegistration register_audio_keep_alive_handler(AudioKeepAliveHandler const& handler) override;

            void idle();
            void active();

            void leftUp();
            void rightUp();

            struct Mock
            {
                MOCK_METHOD0(start_processing, void());
                MOCK_METHOD1(register_audio_headphone_cs_handler, void(AudioHeadphoneCSHandler const&));
                MOCK_METHOD0(unregister_audio_headphone_cs_handler, void());
                MOCK_METHOD1(register_audio_keep_alive_handler, void(AudioKeepAliveHandler const&));
                MOCK_METHOD0(unregister_audio_keep_alive_handler, void());
            };
            testing::NiceMock<Mock> mock;

        private:
            AudioKeepAliveHandler keep_alive_handler;
            AudioHeadphoneCSHandler headphone_cs_handler;
        };

    }
}
