/*
 * Copyright © 2021 Gemian
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

#include <functional>
#include <glib.h>

namespace repowerd
{
	class SessionBusProvider
	{
	public:
		SessionBusProvider();
		~SessionBusProvider();

		void UpdateSessionBus(guint32 bus_address);
		std::string Get();

	private:
		std::string session_bus_address;
	};

}
