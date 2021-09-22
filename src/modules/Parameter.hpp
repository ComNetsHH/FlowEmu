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

#ifndef PARAMETER_HPP
#define PARAMETER_HPP

#include <atomic>
#include <functional>
#include <iostream>
#include <list>
#include <string>

class Parameter {
	public:
		Parameter(double default_value, double min, double max, double step) : value(default_value), min(min), max(max), step(step) {
			
		}

		void addChangeHandler(std::function<void(double)> handler) {
			change_handlers.push_back(handler);
		}

		void callChangeHandlers() {
			for(const auto& handler : change_handlers) {
				try {
					handler(value);
				} catch(const std::bad_function_call &e) {
				}
			}
		}

		void set(double value) {
			if(value < min) {
				std::cerr << "Parameter value of " << value << " subceeds minimum of " << min << "! Parameter value will be set to minimum." << std::endl;
				value = min;
			} else if(value > max) {
				std::cerr << "Parameter value of " << value << " exceeds maximum of " << max << "! Parameter value will be set to maximum." << std::endl;
				value = max;
			}

			this->value.store(value);

			callChangeHandlers();
		}

		double get() {
			return value.load();
		}

		double getMin() {
			return min;
		}

		double getMax() {
			return max;
		}

		double getStep() {
			return step;
		}

	private:
		std::atomic<double> value;

		double min;
		double max;
		double step;

		std::list<std::function<void(double)>> change_handlers;
};

#endif
