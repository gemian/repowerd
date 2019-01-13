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
    enum class SilverButtonState{released, onPressed};
    using SilverButtonHandler = std::function<void(SilverButtonState)>;

    class SilverButton
    {
    public:
        virtual ~SilverButton() = default;

        virtual void start_processing() = 0;

        virtual HandlerRegistration register_silver_button_handler(
                SilverButtonHandler const& handler) = 0;

    protected:
        SilverButton() = default;
        SilverButton (SilverButton const&) = default;
        SilverButton& operator=(SilverButton const&) = default;
    };

}
