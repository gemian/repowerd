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

#include "fake_silver_button.h"

namespace rt = repowerd::test;

void rt::FakeSilverButton::start_processing()
{
    mock.start_processing();
}

repowerd::HandlerRegistration rt::FakeSilverButton::register_silver_button_handler(
        SilverButtonHandler const& handler)
{
    mock.register_silver_button_handler(handler);
    this->handler = handler;
    return HandlerRegistration{
            [this]
            {
                mock.unregister_silver_button_handler();
                this->handler = [](SilverButtonState){};
            }};
}

void rt::FakeSilverButton::release()
{
    handler(SilverButtonState::released);
}

void rt::FakeSilverButton::onPress()
{
    handler(SilverButtonState::onPressed);
}
