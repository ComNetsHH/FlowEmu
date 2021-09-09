/*
 * FlowEmu - Flow-Based Network Emulator
 * Copyright (c) 2021 Institute of Communication Networks (ComNets),
 *                    Hamburg University of Technology (TUHH),
 *                    https://www.tuhh.de/comnets
 * Copyright (c) 2021 Daniel Stolpmann <daniel.stolpmann@tuhh.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef STATISTIC_HPP
#define STATISTIC_HPP

#include <functional>
#include <list>

class Statistic {
	public:
		Statistic() {
			
		}

		void addHandler(std::function<void(double)> handler) {
			handlers.push_back(handler);
		}

		void set(double value) {
			for(const auto& handler : handlers) {
				try {
					handler(value);
				} catch(const std::bad_function_call &e) {
				}
			}
		}

	private:
		std::list<std::function<void(double)>> handlers;
};

#endif
