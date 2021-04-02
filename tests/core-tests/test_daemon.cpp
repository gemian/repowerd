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

#include "daemon_config.h"
#include "run_daemon.h"
#include "fake_client_requests.h"
#include "fake_client_settings.h"
#include "fake_lid.h"
#include "fake_lock.h"
#include "fake_notification_service.h"
#include "fake_power_button.h"
#include "fake_power_source.h"
#include "fake_proximity_sensor.h"
#include "fake_session_tracker.h"
#include "fake_system_power_control.h"
#include "fake_timer.h"
#include "fake_user_activity.h"
#include "fake_voice_call_service.h"
#include "mock_brightness_control.h"

#include "src/core/daemon.h"
#include "src/core/power_button.h"
#include "src/core/state_machine.h"
#include "src/core/state_machine_factory.h"

#include <thread>

#include <gmock/gmock.h>

namespace rt = repowerd::test;

using namespace testing;
using namespace std::chrono_literals;

namespace
{

struct MockStateMachine : public repowerd::StateMachine
{
    MockStateMachine(
        std::shared_ptr<std::string> const& sessions_activity_log,
        std::string const& name)
        : sessions_activity_log{sessions_activity_log}, name{name}
    {
    }

    MOCK_METHOD1(handle_alarm, void(repowerd::AlarmId));

    MOCK_METHOD0(handle_active_call, void());
    MOCK_METHOD0(handle_no_active_call, void());
    MOCK_METHOD1(handle_update_call_state, void(repowerd::OfonoCallState));

    MOCK_METHOD0(handle_no_notification, void());
    MOCK_METHOD0(handle_notification, void());

    MOCK_METHOD0(handle_lid_closed, void());
    MOCK_METHOD0(handle_lid_open, void());
    MOCK_METHOD2(handle_set_lid_behavior,
                 void(repowerd::PowerAction power_action,
                      repowerd::PowerSupply power_supply));

    MOCK_METHOD0(handle_lock_active, void());
    MOCK_METHOD0(handle_lock_inactive, void());

    MOCK_METHOD1(handle_power_button_press, void(repowerd::PowerButtonState));
    MOCK_METHOD0(handle_power_button_release, void());

    MOCK_METHOD0(handle_silver_button_press, void());
    MOCK_METHOD0(handle_silver_button_release, void());

    MOCK_METHOD0(handle_audio_headphone_cs_left_up, void());
    MOCK_METHOD0(handle_audio_headphone_cs_right_up, void());

    MOCK_METHOD0(handle_audio_keep_alive_idle, void());
    MOCK_METHOD0(handle_audio_keep_alive_active, void());

    MOCK_METHOD0(handle_power_source_change, void());
    MOCK_METHOD0(handle_power_source_critical, void());
    MOCK_METHOD1(handle_set_critical_power_behavior,
                 void(repowerd::PowerAction power_action));

    MOCK_METHOD0(handle_proximity_far, void());
    MOCK_METHOD0(handle_proximity_near, void());

    MOCK_METHOD0(handle_enable_inactivity_timeout, void());
    MOCK_METHOD0(handle_disable_inactivity_timeout, void());
    MOCK_METHOD3(handle_set_inactivity_behavior,
                 void(repowerd::PowerAction power_action,
                      repowerd::PowerSupply power_supply,
                      std::chrono::milliseconds timeout));

    MOCK_METHOD0(handle_user_activity_extending_power_state, void());
    MOCK_METHOD0(handle_user_activity_changing_power_state, void());

    MOCK_METHOD1(handle_set_normal_brightness_value, void(double));
    MOCK_METHOD1(handle_modify_normal_brightness_value, void(std::string const&));
    MOCK_METHOD0(handle_enable_autobrightness, void());
    MOCK_METHOD0(handle_disable_autobrightness, void());

    MOCK_METHOD0(handle_system_resume, void());

    MOCK_METHOD0(handle_allow_suspend, void());
    MOCK_METHOD0(handle_disallow_suspend, void());

    void start()
    {
        *sessions_activity_log += " start:" + name;
    }

    void pause()
    {
        *sessions_activity_log += " pause:" + name;
    }

    void resume()
    {
        *sessions_activity_log += " resume:" + name;
    }

    std::shared_ptr<std::string> const sessions_activity_log;
    std::string const name;
};

struct MockStateMachineFactory : public repowerd::StateMachineFactory
{
    MockStateMachineFactory(std::string const& default_name)
        : default_name{default_name}
    {
    }

    std::shared_ptr<repowerd::StateMachine> create_state_machine(std::string const& name)
    {
        if (mock_state_machine && mock_state_machine->name == name)
            mock_state_machines.push_back(std::move(mock_state_machine));
        else
            mock_state_machines.push_back(std::make_shared<MockStateMachine>(sessions_activity_log, name));
        return mock_state_machines.back();
    }

    std::shared_ptr<MockStateMachine> the_mock_state_machine()
    {
        if (!mock_state_machines.empty())
            return mock_state_machines.back();
        else if (mock_state_machine)
            return mock_state_machine;
        else
            return mock_state_machine = std::make_shared<MockStateMachine>(sessions_activity_log, default_name);
    }

    std::shared_ptr<MockStateMachine> the_mock_state_machine(int index)
    {
        return mock_state_machines.at(index);
    }

    std::string const default_name;
    std::shared_ptr<std::string> sessions_activity_log{std::make_shared<std::string>()};
    std::shared_ptr<MockStateMachine> mock_state_machine;
    std::vector<std::shared_ptr<MockStateMachine>> mock_state_machines;
};

struct DaemonConfigWithMockStateMachine : rt::DaemonConfig
{
    std::shared_ptr<repowerd::StateMachineFactory> the_state_machine_factory() override
    {
        return the_mock_state_machine_factory();
    }

    std::shared_ptr<MockStateMachineFactory> the_mock_state_machine_factory()
    {
        if (!mock_state_machine_factory)
        {
            mock_state_machine_factory = std::make_shared<MockStateMachineFactory>(
                the_fake_session_tracker()->default_session());
        }

        return mock_state_machine_factory;
    }

    std::shared_ptr<MockStateMachine> the_mock_state_machine()
    {
        the_state_machine_factory();
        return mock_state_machine_factory->the_mock_state_machine();
    }

    std::shared_ptr<MockStateMachine> the_mock_state_machine(int index)
    {
        return mock_state_machine_factory->the_mock_state_machine(index);
    }

    std::shared_ptr<MockStateMachineFactory> mock_state_machine_factory;
};

struct ADaemon : testing::Test
{
    DaemonConfigWithMockStateMachine config;
    std::unique_ptr<repowerd::Daemon> daemon;
    std::thread daemon_thread;

    ~ADaemon()
    {
        if (daemon_thread.joinable())
            stop_daemon();
    }

    void start_daemon()
    {
        start_daemon_with_config(config);
    }

    void start_daemon_with_config(repowerd::DaemonConfig& config)
    {
        daemon = std::make_unique<repowerd::Daemon>(config);
        daemon_thread = rt::run_daemon(*daemon);
    }

    void stop_daemon()
    {
        daemon->flush();
        daemon->stop();
        daemon_thread.join();
    }

    void flush_daemon()
    {
        daemon->flush();
    }

    void start_daemon_with_second_session_active()
    {
        start_daemon();
        add_session("s1", repowerd::SessionType::RepowerdCompatible, 42);
        switch_to_session("s1");
    }

    void add_session(std::string const& session_id, repowerd::SessionType type, pid_t pid)
    {
        config.the_fake_session_tracker()->add_session(session_id, type, pid);
        daemon->flush();
    }

    void switch_to_session(std::string const& session_id)
    {
        config.the_fake_session_tracker()->switch_to_session(session_id);
        daemon->flush();
    }

    void remove_session(std::string const& session_id)
    {
        config.the_fake_session_tracker()->remove_session(session_id);
        daemon->flush();
    }

    std::string sessions_activity_log()
    {
        return *config.the_mock_state_machine_factory()->sessions_activity_log;
    }
};

}

TEST_F(ADaemon, registers_and_unregisters_timer_alarm_handler)
{
    EXPECT_CALL(config.the_fake_timer()->mock, register_alarm_handler(_));
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_timer().get());

    EXPECT_CALL(config.the_fake_timer()->mock, unregister_alarm_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_timer().get());
}

TEST_F(ADaemon, notifies_state_machine_of_timer_alarm)
{
    start_daemon();

    auto const alarm_id = config.the_fake_timer()->schedule_alarm_in(1s);

    EXPECT_CALL(*config.the_mock_state_machine(), handle_alarm(alarm_id));

    config.the_fake_timer()->advance_by(1s);
}

TEST_F(ADaemon, registers_starts_and_unregisters_power_button_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_power_button()->mock, register_power_button_handler(_));
    EXPECT_CALL(config.the_fake_power_button()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_power_button().get());

    EXPECT_CALL(config.the_fake_power_button()->mock, unregister_power_button_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_power_button().get());
}

TEST_F(ADaemon, notifies_state_machine_of_power_on_button_press)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_power_button_press(repowerd::PowerButtonState::onPressed));
    config.the_fake_power_button()->onPress();
}

TEST_F(ADaemon, notifies_state_machine_of_power_sleep_button_press)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_power_button_press(repowerd::PowerButtonState::sleepPressed));
    config.the_fake_power_button()->sleepPress();
}

TEST_F(ADaemon, notifies_state_machine_of_power_button_release)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_power_button_release());
    config.the_fake_power_button()->release();
}

TEST_F(ADaemon, registers_starts_and_unregisters_lock_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_lock()->mock, register_lock_handler(_));
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_lock().get());

    EXPECT_CALL(config.the_fake_lock()->mock, unregister_lock_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_lock().get());
}

TEST_F(ADaemon, notifies_state_machine_of_lock_active)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_lock_active());
    config.the_fake_lock()->active();
}

TEST_F(ADaemon, notifies_state_machine_of_lock_inactive)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_lock_inactive());
    config.the_fake_lock()->inactive();
}

TEST_F(ADaemon, registers_starts_and_unregisters_user_activity_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_user_activity()->mock, register_user_activity_handler(_));
    EXPECT_CALL(config.the_fake_user_activity()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_user_activity().get());

    EXPECT_CALL(config.the_fake_user_activity()->mock, unregister_user_activity_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_user_activity().get());
}

TEST_F(ADaemon, notifies_state_machine_of_user_activity_changing_power_state)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_user_activity_changing_power_state());

    config.the_fake_user_activity()->perform(repowerd::UserActivityType::change_power_state);
}

TEST_F(ADaemon, notifies_state_machine_of_user_activity_extending_power_state)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_user_activity_extending_power_state());

    config.the_fake_user_activity()->perform(repowerd::UserActivityType::extend_power_state);
}

TEST_F(ADaemon, registers_starts_and_unregisters_proximity_handler)
{
    EXPECT_CALL(config.the_fake_proximity_sensor()->mock, register_proximity_handler(_));
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_proximity_sensor().get());

    EXPECT_CALL(config.the_fake_proximity_sensor()->mock, unregister_proximity_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_proximity_sensor().get());
}

TEST_F(ADaemon, notifies_state_machine_of_proximity_far)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_proximity_far());

    config.the_fake_proximity_sensor()->emit_proximity_state(repowerd::ProximityState::far);
}

TEST_F(ADaemon, notifies_state_machine_of_proximity_near)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_proximity_near());

    config.the_fake_proximity_sensor()->emit_proximity_state(repowerd::ProximityState::near);
}

TEST_F(ADaemon, registers_starts_and_unregisters_enable_inactivity_timeout_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_client_requests()->mock, register_enable_inactivity_timeout_handler(_));
    EXPECT_CALL(config.the_fake_client_requests()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_client_requests().get());

    EXPECT_CALL(config.the_fake_client_requests()->mock, unregister_enable_inactivity_timeout_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_client_requests().get());
}

TEST_F(ADaemon, notifies_state_machine_of_enable_inactivity_timeout)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_enable_inactivity_timeout());

    config.the_fake_client_requests()->emit_enable_inactivity_timeout("id");
}

TEST_F(ADaemon, notifies_inactive_state_machine_of_enable_inactivity_timeout)
{
    start_daemon_with_second_session_active();

    EXPECT_CALL(*config.the_mock_state_machine(0), handle_enable_inactivity_timeout());
    EXPECT_CALL(*config.the_mock_state_machine(1), handle_enable_inactivity_timeout()).Times(0);
    config.the_fake_client_requests()->emit_enable_inactivity_timeout("id");
}

TEST_F(ADaemon, registers_and_unregisters_disable_inactivity_timeout_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_client_requests()->mock, register_disable_inactivity_timeout_handler(_));
    EXPECT_CALL(config.the_fake_client_requests()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_client_requests().get());

    EXPECT_CALL(config.the_fake_client_requests()->mock, unregister_disable_inactivity_timeout_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_client_requests().get());
}

TEST_F(ADaemon, notifies_state_machine_of_disable_inactivity_timeout)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_disable_inactivity_timeout());

    config.the_fake_client_requests()->emit_disable_inactivity_timeout("id");
}

TEST_F(ADaemon, notifies_inactive_state_machine_of_disable_inactivity_timeout)
{
    start_daemon_with_second_session_active();

    EXPECT_CALL(*config.the_mock_state_machine(0), handle_disable_inactivity_timeout());
    EXPECT_CALL(*config.the_mock_state_machine(1), handle_disable_inactivity_timeout()).Times(0);
    config.the_fake_client_requests()->emit_disable_inactivity_timeout("id");
}

TEST_F(ADaemon, registers_and_unregisters_set_inactivity_timeout_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_client_requests()->mock, register_set_inactivity_timeout_handler(_));
    EXPECT_CALL(config.the_fake_client_requests()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_client_requests().get());

    EXPECT_CALL(config.the_fake_client_requests()->mock, unregister_set_inactivity_timeout_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_client_requests().get());
}

TEST_F(ADaemon, notifies_state_machine_of_set_inactivity_timeout)
{
    start_daemon();

    auto const timeout = 10000ms;
    EXPECT_CALL(*config.the_mock_state_machine(),
                handle_set_inactivity_behavior(
                    repowerd::PowerAction::display_off,
                    repowerd::PowerSupply::battery,
                    timeout));
    EXPECT_CALL(*config.the_mock_state_machine(),
                handle_set_inactivity_behavior(
                    repowerd::PowerAction::display_off,
                    repowerd::PowerSupply::line_power,
                    timeout));

    config.the_fake_client_requests()->emit_set_inactivity_timeout(timeout);
}

TEST_F(ADaemon, notifies_inactive_state_machine_of_set_inactivity_timeout)
{
    start_daemon_with_second_session_active();

    auto const timeout = 10000ms;
    EXPECT_CALL(*config.the_mock_state_machine(0),
                handle_set_inactivity_behavior(
                    repowerd::PowerAction::display_off,
                    _,
                    timeout)).Times(2);
    EXPECT_CALL(*config.the_mock_state_machine(1),
                handle_set_inactivity_behavior(_, _, _)).Times(0);
    config.the_fake_client_requests()->emit_set_inactivity_timeout(timeout);
}

TEST_F(ADaemon, registers_and_unregisters_disable_autobrightness)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_client_requests()->mock, register_disable_autobrightness_handler(_));
    EXPECT_CALL(config.the_fake_client_requests()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_client_requests().get());

    EXPECT_CALL(config.the_fake_client_requests()->mock, unregister_disable_autobrightness_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_client_requests().get());
}

TEST_F(ADaemon, notifies_state_machine_of_disable_autobrightness)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_disable_autobrightness());

    config.the_fake_client_requests()->emit_disable_autobrightness();
}

TEST_F(ADaemon, notifies_inactive_state_machine_of_disable_autobrightness)
{
    start_daemon_with_second_session_active();

    EXPECT_CALL(*config.the_mock_state_machine(0), handle_disable_autobrightness());
    EXPECT_CALL(*config.the_mock_state_machine(1), handle_disable_autobrightness()).Times(0);
    config.the_fake_client_requests()->emit_disable_autobrightness();
}

TEST_F(ADaemon, registers_and_unregisters_enable_autobrightness)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_client_requests()->mock, register_enable_autobrightness_handler(_));
    EXPECT_CALL(config.the_fake_client_requests()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_client_requests().get());

    EXPECT_CALL(config.the_fake_client_requests()->mock, unregister_enable_autobrightness_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_client_requests().get());
}

TEST_F(ADaemon, notifies_state_machine_of_enable_autobrightness)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_enable_autobrightness());

    config.the_fake_client_requests()->emit_enable_autobrightness();
}

TEST_F(ADaemon, notifies_inactive_state_machine_of_enable_autobrightness)
{
    start_daemon_with_second_session_active();

    EXPECT_CALL(*config.the_mock_state_machine(0), handle_enable_autobrightness());
    EXPECT_CALL(*config.the_mock_state_machine(1), handle_enable_autobrightness()).Times(0);
    config.the_fake_client_requests()->emit_enable_autobrightness();
}

TEST_F(ADaemon, registers_and_unregisters_set_normal_brightness_value)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_client_requests()->mock, register_set_normal_brightness_value_handler(_));
    EXPECT_CALL(config.the_fake_client_requests()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_client_requests().get());

    EXPECT_CALL(config.the_fake_client_requests()->mock, unregister_set_normal_brightness_value_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_client_requests().get());
}

TEST_F(ADaemon, notifies_state_machine_of_set_normal_brightness_value)
{
    start_daemon();

    auto const value = 0.7;
    EXPECT_CALL(*config.the_mock_state_machine(), handle_set_normal_brightness_value(value));

    config.the_fake_client_requests()->emit_set_normal_brightness_value(value);
}

TEST_F(ADaemon, notifies_inactive_state_machine_of_set_normal_brightness_value)
{
    start_daemon_with_second_session_active();

    auto const value = 0.7;
    EXPECT_CALL(*config.the_mock_state_machine(0), handle_set_normal_brightness_value(value));
    EXPECT_CALL(*config.the_mock_state_machine(1), handle_set_normal_brightness_value(_)).Times(0);

    config.the_fake_client_requests()->emit_set_normal_brightness_value(value);
}

TEST_F(ADaemon, registers_starts_and_unregisters_allow_suspend_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_client_requests()->mock, register_allow_suspend_handler(_));
    EXPECT_CALL(config.the_fake_client_requests()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_client_requests().get());

    EXPECT_CALL(config.the_fake_client_requests()->mock, unregister_allow_suspend_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_client_requests().get());
}

TEST_F(ADaemon, notifies_state_machine_of_allow_suspend_handler)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_allow_suspend());

    config.the_fake_client_requests()->emit_allow_suspend("id");
}

TEST_F(ADaemon, notifies_inactive_state_machine_of_allow_suspend)
{
    start_daemon_with_second_session_active();

    EXPECT_CALL(*config.the_mock_state_machine(0), handle_allow_suspend());
    EXPECT_CALL(*config.the_mock_state_machine(1), handle_allow_suspend()).Times(0);
    config.the_fake_client_requests()->emit_allow_suspend("id");
}

TEST_F(ADaemon, registers_starts_and_unregisters_disallow_suspend_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_client_requests()->mock, register_disallow_suspend_handler(_));
    EXPECT_CALL(config.the_fake_client_requests()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_client_requests().get());

    EXPECT_CALL(config.the_fake_client_requests()->mock, unregister_disallow_suspend_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_client_requests().get());
}

TEST_F(ADaemon, notifies_state_machine_of_disallow_suspend_handler)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_disallow_suspend());

    config.the_fake_client_requests()->emit_disallow_suspend("id");
}

TEST_F(ADaemon, notifies_inactive_state_machine_of_disallow_suspend)
{
    start_daemon_with_second_session_active();

    EXPECT_CALL(*config.the_mock_state_machine(0), handle_disallow_suspend());
    EXPECT_CALL(*config.the_mock_state_machine(1), handle_disallow_suspend()).Times(0);
    config.the_fake_client_requests()->emit_disallow_suspend("id");
}

TEST_F(ADaemon, registers_and_unregisters_notification_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_notification_service()->mock, register_notification_handler(_));
    EXPECT_CALL(config.the_fake_notification_service()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_notification_service().get());

    EXPECT_CALL(config.the_fake_notification_service()->mock, unregister_notification_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_notification_service().get());
}

TEST_F(ADaemon, notifies_state_machine_of_notification)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_notification());

    config.the_fake_notification_service()->emit_notification("id");
}

TEST_F(ADaemon, notifies_inactive_state_machine_of_notification)
{
    start_daemon_with_second_session_active();

    EXPECT_CALL(*config.the_mock_state_machine(0), handle_notification());
    EXPECT_CALL(*config.the_mock_state_machine(1), handle_notification()).Times(0);

    config.the_fake_notification_service()->emit_notification("id");
}

TEST_F(ADaemon, registers_and_unregisters_no_notification_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_notification_service()->mock, register_notification_done_handler(_));
    EXPECT_CALL(config.the_fake_notification_service()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_notification_service().get());

    EXPECT_CALL(config.the_fake_notification_service()->mock, unregister_notification_done_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_notification_service().get());
}

TEST_F(ADaemon, notifies_state_machine_of_no_notification)
{
    start_daemon();

    InSequence s;
    EXPECT_CALL(*config.the_mock_state_machine(), handle_notification());
    EXPECT_CALL(*config.the_mock_state_machine(), handle_no_notification());

    config.the_fake_notification_service()->emit_notification("id");
    config.the_fake_notification_service()->emit_notification_done("id");
}

TEST_F(ADaemon, notifies_inactive_state_machine_of_no_notification)
{
    start_daemon_with_second_session_active();

    InSequence s;
    EXPECT_CALL(*config.the_mock_state_machine(0), handle_notification());
    EXPECT_CALL(*config.the_mock_state_machine(0), handle_no_notification());
    EXPECT_CALL(*config.the_mock_state_machine(1), handle_no_notification()).Times(0);

    config.the_fake_notification_service()->emit_notification("id");
    config.the_fake_notification_service()->emit_notification_done("id");
}

TEST_F(ADaemon, notifies_inactive_state_machine_of_no_notification_if_notification_was_sent_while_active)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(0), handle_notification());
    config.the_fake_notification_service()->emit_notification("id");
    flush_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_voice_call_service().get());

    add_session("s1", repowerd::SessionType::RepowerdCompatible, 42);
    switch_to_session("s1");

    InSequence s;
    EXPECT_CALL(*config.the_mock_state_machine(0), handle_no_notification());
    EXPECT_CALL(*config.the_mock_state_machine(1), handle_no_notification()).Times(0);

    config.the_fake_notification_service()->emit_notification_done("id");
}

TEST_F(ADaemon, registers_and_unregisters_active_call_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_voice_call_service()->mock, register_active_call_handler(_));
    EXPECT_CALL(config.the_fake_voice_call_service()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_voice_call_service().get());

    EXPECT_CALL(config.the_fake_voice_call_service()->mock, unregister_active_call_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_voice_call_service().get());
}

TEST_F(ADaemon, notifies_state_machine_of_active_call)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_active_call());

    config.the_fake_voice_call_service()->emit_active_call();
}

TEST_F(ADaemon, registers_and_unregisters_no_active_call_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_voice_call_service()->mock, register_no_active_call_handler(_));
    EXPECT_CALL(config.the_fake_voice_call_service()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_voice_call_service().get());

    EXPECT_CALL(config.the_fake_voice_call_service()->mock, unregister_no_active_call_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_voice_call_service().get());
}

TEST_F(ADaemon, notifies_state_machine_of_no_active_call)
{
    start_daemon();

    InSequence s;
    EXPECT_CALL(*config.the_mock_state_machine(), handle_active_call());
    EXPECT_CALL(*config.the_mock_state_machine(), handle_no_active_call());

    config.the_fake_voice_call_service()->emit_active_call();
    config.the_fake_voice_call_service()->emit_no_active_call();
}

TEST_F(ADaemon, registers_and_unregisters_power_source_change_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_power_source()->mock, register_power_source_change_handler(_));
    EXPECT_CALL(config.the_fake_power_source()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_power_source().get());

    EXPECT_CALL(config.the_fake_power_source()->mock, unregister_power_source_change_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_power_source().get());
}

TEST_F(ADaemon, notifies_state_machine_of_power_source_change)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_power_source_change());

    config.the_fake_power_source()->emit_power_source_change();
}

TEST_F(ADaemon, registers_and_unregisters_power_source_critical_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_power_source()->mock, register_power_source_critical_handler(_));
    EXPECT_CALL(config.the_fake_power_source()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_power_source().get());

    EXPECT_CALL(config.the_fake_power_source()->mock, unregister_power_source_critical_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_power_source().get());
}

TEST_F(ADaemon, notifies_state_machine_of_power_source_critical)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_power_source_critical());

    config.the_fake_power_source()->emit_power_source_critical();
}

TEST_F(ADaemon, registers_and_unregisters_active_session_changed_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_session_tracker()->mock,
                register_active_session_changed_handler(_));
    EXPECT_CALL(config.the_fake_session_tracker()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_session_tracker().get());

    EXPECT_CALL(config.the_fake_session_tracker()->mock,
                unregister_active_session_changed_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_session_tracker().get());
}

TEST_F(ADaemon, registers_and_unregisters_session_removed_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_session_tracker()->mock, register_session_removed_handler(_));
    EXPECT_CALL(config.the_fake_session_tracker()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_session_tracker().get());

    EXPECT_CALL(config.the_fake_session_tracker()->mock, unregister_session_removed_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_session_tracker().get());
}

TEST_F(ADaemon, starts_session_tracker_processing_before_per_session_components)
{
    Expectation session = EXPECT_CALL(config.the_fake_session_tracker()->mock, start_processing());
    EXPECT_CALL(config.the_fake_client_requests()->mock, start_processing())
        .After(session);
    EXPECT_CALL(config.the_fake_notification_service()->mock, start_processing())
        .After(session);

    start_daemon();
}

TEST_F(ADaemon, makes_null_session_active_if_active_is_removed)
{
    start_daemon();

    remove_session(
        config.the_fake_session_tracker()->default_session());

    EXPECT_CALL(*config.the_mock_state_machine(), handle_power_button_press(_)).Times(0);
    config.the_fake_power_button()->onPress();
}

TEST_F(ADaemon, pauses_active_session_before_removing_it)
{
    auto const start = [](std::string name) { return " start:" + name; };
    auto const pause = [](std::string name) { return " pause:" + name; };

    start_daemon();
    auto const default_session = config.the_fake_session_tracker()->default_session();

    remove_session(
        config.the_fake_session_tracker()->default_session());

    EXPECT_THAT(sessions_activity_log(),
                StrEq(start(default_session) +
                      pause(default_session)));
}

TEST_F(ADaemon, starts_session_on_first_switch)
{
    start_daemon();

    add_session("s1", repowerd::SessionType::RepowerdCompatible, 42);
    switch_to_session("s1");

    EXPECT_THAT(sessions_activity_log(), HasSubstr("start:s1"));
    EXPECT_THAT(sessions_activity_log(), Not(HasSubstr("resume:s1")));
}

TEST_F(ADaemon, starts_pauses_resumes_sessions_on_switch)
{
    auto const start = [](std::string name) { return " start:" + name; };
    auto const pause = [](std::string name) { return " pause:" + name; };
    auto const resume = [](std::string name) { return " resume:" + name; };

    start_daemon();
    auto const default_session = config.the_fake_session_tracker()->default_session();

    add_session("s1", repowerd::SessionType::RepowerdCompatible, 42);
    switch_to_session("s1");

    EXPECT_THAT(sessions_activity_log(),
                StrEq(start(default_session) +
                      pause(default_session) +
                      start("s1")));

    switch_to_session(default_session);

    EXPECT_THAT(sessions_activity_log(),
                StrEq(start(default_session) +
                      pause(default_session) +
                      start("s1") +
                      pause("s1") +
                      resume(default_session)));
}

TEST_F(ADaemon, registers_starts_and_unregisters_lid_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_lid()->mock, register_lid_handler(_));
    EXPECT_CALL(config.the_fake_lid()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_lid().get());

    EXPECT_CALL(config.the_fake_lid()->mock, unregister_lid_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_lid().get());
}

TEST_F(ADaemon, notifies_state_machine_of_lid_closed)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_lid_closed());
    config.the_fake_lid()->close();
}

TEST_F(ADaemon, notifies_state_machine_of_lid_open)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_lid_open());
    config.the_fake_lid()->open();
}

TEST_F(ADaemon, registers_and_unregisters_set_inactivity_behavior_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_client_settings()->mock, register_set_inactivity_behavior_handler(_));
    EXPECT_CALL(config.the_fake_client_settings()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_client_settings().get());

    EXPECT_CALL(config.the_fake_client_settings()->mock, unregister_set_inactivity_behavior_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_client_settings().get());
}

TEST_F(ADaemon, notifies_state_machine_of_set_inactivity_behavior)
{
    start_daemon();

    auto const power_action = repowerd::PowerAction::display_off;
    auto const power_supply = repowerd::PowerSupply::line_power;
    auto const timeout = 10000ms;

    EXPECT_CALL(*config.the_mock_state_machine(),
                handle_set_inactivity_behavior(power_action, power_supply, timeout));

    config.the_fake_client_settings()->emit_set_inactivity_behavior(
        power_action, power_supply, timeout);
}

TEST_F(ADaemon, notifies_inactive_state_machine_of_set_inactivity_behavior)
{
    start_daemon_with_second_session_active();

    auto const power_action = repowerd::PowerAction::display_off;
    auto const power_supply = repowerd::PowerSupply::line_power;
    auto const timeout = 10000ms;

    EXPECT_CALL(*config.the_mock_state_machine(0),
                handle_set_inactivity_behavior(power_action, power_supply, timeout));
    EXPECT_CALL(*config.the_mock_state_machine(1),
                handle_set_inactivity_behavior(_, _, _)).Times(0);

    config.the_fake_client_settings()->emit_set_inactivity_behavior(
        power_action, power_supply, timeout);
}

TEST_F(ADaemon, registers_and_unregisters_set_lid_behavior_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_client_settings()->mock, register_set_lid_behavior_handler(_));
    EXPECT_CALL(config.the_fake_client_settings()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_client_settings().get());

    EXPECT_CALL(config.the_fake_client_settings()->mock, unregister_set_lid_behavior_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_client_settings().get());
}

TEST_F(ADaemon, notifies_state_machine_of_set_lid_behavior)
{
    start_daemon();

    auto const power_action = repowerd::PowerAction::display_off;
    auto const power_supply = repowerd::PowerSupply::line_power;

    EXPECT_CALL(*config.the_mock_state_machine(),
                handle_set_lid_behavior(power_action, power_supply));

    config.the_fake_client_settings()->emit_set_lid_behavior(
        power_action, power_supply);
}

TEST_F(ADaemon, notifies_inactive_state_machine_of_set_lid_behavior)
{
    start_daemon_with_second_session_active();

    auto const power_action = repowerd::PowerAction::display_off;
    auto const power_supply = repowerd::PowerSupply::line_power;

    EXPECT_CALL(*config.the_mock_state_machine(0),
                handle_set_lid_behavior(power_action, power_supply));
    EXPECT_CALL(*config.the_mock_state_machine(1),
                handle_set_lid_behavior(_, _)).Times(0);

    config.the_fake_client_settings()->emit_set_lid_behavior(
        power_action, power_supply);
}

TEST_F(ADaemon, registers_and_unregisters_set_critical_power_behavior_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_client_settings()->mock,
                register_set_critical_power_behavior_handler(_));
    EXPECT_CALL(config.the_fake_client_settings()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_client_settings().get());

    EXPECT_CALL(config.the_fake_client_settings()->mock,
                unregister_set_critical_power_behavior_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_client_settings().get());
}

TEST_F(ADaemon, notifies_state_machine_of_set_critical_power_behavior)
{
    start_daemon();

    auto const power_action = repowerd::PowerAction::suspend;

    EXPECT_CALL(*config.the_mock_state_machine(),
                handle_set_critical_power_behavior(power_action));

    config.the_fake_client_settings()->emit_set_critical_power_behavior(
        power_action);
}

TEST_F(ADaemon, notifies_inactive_state_machine_of_set_critical_power_behavior)
{
    start_daemon_with_second_session_active();

    auto const power_action = repowerd::PowerAction::suspend;

    EXPECT_CALL(*config.the_mock_state_machine(0),
                handle_set_critical_power_behavior(power_action));
    EXPECT_CALL(*config.the_mock_state_machine(1),
                handle_set_critical_power_behavior(_)).Times(0);

    config.the_fake_client_settings()->emit_set_critical_power_behavior(
        power_action);
}

TEST_F(ADaemon, registers_and_unregisters_system_resume_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_system_power_control()->mock, register_system_resume_handler(_));
    EXPECT_CALL(config.the_fake_system_power_control()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_system_power_control().get());

    EXPECT_CALL(config.the_fake_system_power_control()->mock, unregister_system_resume_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_system_power_control().get());
}

TEST_F(ADaemon, notifies_state_machine_of_system_resume)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_system_resume());

    config.the_fake_system_power_control()->emit_system_resume();
}

TEST_F(ADaemon, registers_and_unregisters_system_allow_suspend_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_system_power_control()->mock, register_system_allow_suspend_handler(_));
    EXPECT_CALL(config.the_fake_system_power_control()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_system_power_control().get());

    EXPECT_CALL(config.the_fake_system_power_control()->mock, unregister_system_allow_suspend_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_system_power_control().get());
}

TEST_F(ADaemon, notifies_all_state_machines_of_system_allow_suspend)
{
    start_daemon_with_second_session_active();

    EXPECT_CALL(*config.the_mock_state_machine(0), handle_allow_suspend());
    EXPECT_CALL(*config.the_mock_state_machine(1), handle_allow_suspend());

    config.the_fake_system_power_control()->emit_system_allow_suspend();
}

TEST_F(ADaemon, registers_and_unregisters_system_disallow_suspend_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_system_power_control()->mock, register_system_disallow_suspend_handler(_));
    EXPECT_CALL(config.the_fake_system_power_control()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_system_power_control().get());

    EXPECT_CALL(config.the_fake_system_power_control()->mock, unregister_system_disallow_suspend_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_system_power_control().get());
}

TEST_F(ADaemon, notifies_all_state_machines_of_system_disallow_suspend)
{
    start_daemon_with_second_session_active();

    EXPECT_CALL(*config.the_mock_state_machine(0), handle_disallow_suspend());
    EXPECT_CALL(*config.the_mock_state_machine(1), handle_disallow_suspend());

    config.the_fake_system_power_control()->emit_system_disallow_suspend();
}
