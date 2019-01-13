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

namespace repowerd
{

    class CallControl
    {
    public:
        virtual ~CallControl() = default;

        virtual void hang_up_and_accept_call() = 0;

    protected:
        CallControl() = default;
        CallControl (CallControl const&) = default;
        CallControl& operator=(CallControl const&) = default;
    };

}

