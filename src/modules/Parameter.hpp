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

#include <json/json.h>

class Parameter {
	public:
		virtual void callChangeHandlers() = 0;

		virtual Json::Value serialize() = 0;
};

template<typename T> class ParameterTemplate : public Parameter {
	public:
		ParameterTemplate(T default_value) : value(default_value) {
			
		}

		void addChangeHandler(std::function<void(T)> handler) {
			change_handlers.push_back(handler);
		}

		void callChangeHandlers() override {
			for(const auto& handler : change_handlers) {
				try {
					handler(value);
				} catch(const std::bad_function_call &e) {
				}
			}
		}

		virtual void set(T value) {
			this->value.store(value);

			callChangeHandlers();
		}

		virtual T get() {
			return value.load();
		}

		Json::Value serialize() override {
			Json::Value json_root;
			json_root["value"] = get();

			return json_root;
		}

	private:
		std::atomic<T> value;

		std::list<std::function<void(T)>> change_handlers;
};

class ParameterDouble : public ParameterTemplate<double> {
	public:
		ParameterDouble(double default_value, double min, double max, double step) : ParameterTemplate(default_value), min(min), max(max), step(step) {
			
		}

		void set(double value) override {
			if(value < min) {
				std::cerr << "Parameter value of " << value << " subceeds minimum of " << min << "! Parameter value will be set to minimum." << std::endl;
				value = min;
			} else if(value > max) {
				std::cerr << "Parameter value of " << value << " exceeds maximum of " << max << "! Parameter value will be set to maximum." << std::endl;
				value = max;
			}

			ParameterTemplate::set(value);
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

		Json::Value serialize() override {
			Json::Value json_root = ParameterTemplate::serialize();
			json_root["data_type"] = "double";
			json_root["min"] = getMin();
			json_root["max"] = getMax();
			json_root["step"] = getStep();

			return json_root;
		}

	private:
		double min;
		double max;
		double step;
};

#endif
