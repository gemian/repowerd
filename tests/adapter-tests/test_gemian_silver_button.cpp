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

#include "src/adapters/ofono_call_control.h"

#include "dbus_bus.h"
#include "fake_log.h"
#include "fake_ofono.h"
#include "fake_shared.h"
#include "spin_wait.h"
#include "wait_condition.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <chrono>
#include <algorithm>
#include <src/adapters/gemian_silver_button.h>

namespace rt = repowerd::test;
using namespace std::chrono_literals;
using namespace testing;

namespace
{

    struct GemianSilverButtonDBusClient : rt::DBusClient
    {
        GemianSilverButtonDBusClient(std::string const& dbus_address)
                : rt::DBusClient{
                dbus_address,
                "org.thinkglobally.Gemian.SilverButton",
                "/org/thinkglobally/Gemian/SilverButton"}
        {
            connection.request_name("org.thinkglobally.Gemian.SilverButton");
        }

        void emit_silver_button_press()
        {
            emit_signal("org.thinkglobally.Gemian.SilverButton", "OnPress", nullptr);
        }

        void emit_silver_button_release()
        {
            emit_signal("org.thinkglobally.Gemian.SilverButton", "OnRelease", nullptr);
        }

    };

    struct AGemianSilverButton : testing::Test
    {
        AGemianSilverButton()
        {
            registrations.push_back(
                    gemian_silver_button.register_silver_button_handler(
                            [this] (repowerd::SilverButtonState state) { mock_handlers.silver_button(state); }));

            gemian_silver_button.start_processing();
        }

        struct MockHandlers
        {
            MOCK_METHOD1(silver_button, void(repowerd::SilverButtonState));
        };
        testing::NiceMock<MockHandlers> mock_handlers;

        rt::DBusBus bus;
        rt::FakeLog fake_log;

        repowerd::GemianSilverButton gemian_silver_button{bus.address()};
        GemianSilverButtonDBusClient client{bus.address()};

        std::vector<repowerd::HandlerRegistration> registrations;
        std::chrono::seconds const default_timeout{3};
    };

}

TEST_F(AGemianSilverButton, silver_button_press_signal)
{
    rt::WaitCondition request_processed;

    EXPECT_CALL(mock_handlers, silver_button(repowerd::SilverButtonState::onPressed))
            .WillOnce(WakeUp(&request_processed));

    client.emit_silver_button_press();

    request_processed.wait_for(default_timeout);
    EXPECT_TRUE(request_processed.woken());
}
