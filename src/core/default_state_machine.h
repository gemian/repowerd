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

#pragma once

#include "state_machine.h"
#include "daemon_config.h"

#include <array>

namespace repowerd
{

class DefaultStateMachine : public StateMachine
{
public:
    DefaultStateMachine(DaemonConfig& config);

    void handle_alarm(AlarmId id) override;

    void handle_active_call() override;
    void handle_no_active_call() override;

    void handle_enable_inactivity_timeout() override;
    void handle_disable_inactivity_timeout() override;
    void handle_set_inactivity_timeout(std::chrono::milliseconds timeout) override;

    void handle_no_notification() override;
    void handle_notification() override;

    void handle_power_button_press() override;
    void handle_power_button_release() override;

    void handle_proximity_far() override;
    void handle_proximity_near() override;

    void handle_user_activity_changing_power_state() override;
    void handle_user_activity_extending_power_state() override;

private:
    enum class DisplayPowerMode {unknown, on, off};
    struct InactivityTimeoutAllowanceEnum {
        enum Allowance {client, notification, count};
    };
    using InactivityTimeoutAllowance = InactivityTimeoutAllowanceEnum::Allowance;

    void cancel_user_inactivity_alarm();
    void schedule_normal_user_inactivity_alarm();
    void schedule_reduced_user_inactivity_alarm();
    void turn_off_display();
    void turn_on_display_with_normal_timeout();
    void turn_on_display_without_timeout();
    void brighten_display();
    void dim_display();
    void allow_inactivity_timeout(InactivityTimeoutAllowance allowance);
    void disallow_inactivity_timeout(InactivityTimeoutAllowance allowance);
    bool is_inactivity_timeout_allowed();

    std::shared_ptr<BrightnessControl> const brightness_control;
    std::shared_ptr<DisplayPowerControl> const display_power_control;
    std::shared_ptr<PowerButtonEventSink> const power_button_event_sink;
    std::shared_ptr<ProximitySensor> const proximity_sensor;
    std::shared_ptr<Timer> const timer;

    std::array<bool,InactivityTimeoutAllowance::count> inactivity_timeout_allowances;
    DisplayPowerMode display_power_mode;
    DisplayPowerMode display_power_mode_at_power_button_press;
    AlarmId power_button_long_press_alarm_id;
    bool power_button_long_press_detected;
    std::chrono::milliseconds power_button_long_press_timeout;
    AlarmId user_inactivity_display_dim_alarm_id;
    AlarmId user_inactivity_display_off_alarm_id;
    std::chrono::steady_clock::time_point user_inactivity_display_off_time_point;
    std::chrono::milliseconds const user_inactivity_normal_display_dim_duration;
    std::chrono::milliseconds user_inactivity_normal_display_off_timeout;
    std::chrono::milliseconds const user_inactivity_reduced_display_off_timeout;
};

}