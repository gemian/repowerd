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

#include <string>
#include <utility>
#include "session_bus_provider.h"

repowerd::SessionBusProvider::SessionBusProvider() = default;

repowerd::SessionBusProvider::~SessionBusProvider() = default;

void repowerd::SessionBusProvider::UpdateSessionBus(guint32 bus_address) {
    session_bus_address = std::string("unix:path=/run/user/")+std::to_string(bus_address)+std::string("/bus");
}

std::string repowerd::SessionBusProvider::Get() {
	return session_bus_address;
}
