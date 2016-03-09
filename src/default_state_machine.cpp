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

#include "default_state_machine.h"

#include "display_power_control.h"
#include "power_button_event_sink.h"
#include "timer.h"

repowerd::DefaultStateMachine::DefaultStateMachine(DaemonConfig& config)
    : display_power_control{config.the_display_power_control()},
      power_button_event_sink{config.the_power_button_event_sink()},
      timer{config.the_timer()},
      display_power_mode{DisplayPowerMode::off},
      display_power_mode_at_power_button_press{DisplayPowerMode::unknown},
      power_button_long_press_alarm_id{AlarmId::invalid},
      power_button_long_press_detected{false},
      power_button_long_press_timeout{config.power_button_long_press_timeout()},
      user_inactivity_display_off_alarm_id{AlarmId::invalid},
      user_inactivity_display_off_timeout{config.user_inactivity_display_off_timeout()}
{
}

void repowerd::DefaultStateMachine::handle_alarm(AlarmId id)
{
    if (id == power_button_long_press_alarm_id)
    {
        power_button_long_press_detected = true;
        power_button_long_press_alarm_id = AlarmId::invalid;
    }
    else if (id == user_inactivity_display_off_alarm_id)
    {
        set_display_power_mode(DisplayPowerMode::off);
    }
}

void repowerd::DefaultStateMachine::handle_power_button_press()
{
    display_power_mode_at_power_button_press = display_power_mode;

    if (display_power_mode == DisplayPowerMode::off)
    {
        set_display_power_mode(DisplayPowerMode::on);
    }

    power_button_long_press_alarm_id =
        timer->schedule_alarm_in(power_button_long_press_timeout);
}

void repowerd::DefaultStateMachine::handle_power_button_release()
{
    if (power_button_long_press_detected)
    {
        power_button_event_sink->notify_long_press();
        power_button_long_press_detected = false;
    }
    else if (display_power_mode_at_power_button_press == DisplayPowerMode::on)
    {
        set_display_power_mode(DisplayPowerMode::off);
    }

    display_power_mode_at_power_button_press = DisplayPowerMode::unknown;
    power_button_long_press_alarm_id = AlarmId::invalid;
}

void repowerd::DefaultStateMachine::set_display_power_mode(DisplayPowerMode mode)
{
    if (mode == DisplayPowerMode::off)
    {
        display_power_control->turn_off();
        display_power_mode = DisplayPowerMode::off;
        cancel_user_inactivity_alarm();
    }
    else if (mode == DisplayPowerMode::on)
    {
        display_power_control->turn_on();
        display_power_mode = DisplayPowerMode::on;
        schedule_user_inactivity_alarm();
    }
}

void repowerd::DefaultStateMachine::schedule_user_inactivity_alarm()
{
    user_inactivity_display_off_alarm_id =
        timer->schedule_alarm_in(user_inactivity_display_off_timeout);
}

void repowerd::DefaultStateMachine::cancel_user_inactivity_alarm()
{
    user_inactivity_display_off_alarm_id = AlarmId::invalid;
}
