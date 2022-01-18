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
#include <mutex>
#include <string>

#include <json/json.h>

class Parameter {
	public:
		virtual void callChangeHandlers() = 0;

		virtual Json::Value serialize() = 0;
};

template<typename T> class AtomicParameterTemplate;

template<typename T> class ParameterTemplate : public Parameter {
	friend class AtomicParameterTemplate<T>;

	public:
		ParameterTemplate() {
			
		}

		ParameterTemplate(T default_value) : value(default_value) {
			
		}

		void addChangeHandler(std::function<void(T)> handler) {
			change_handlers.push_back(handler);
		}

		void callChangeHandlers() override {
			for(const auto& handler : change_handlers) {
				try {
					handler(get());
				} catch(const std::bad_function_call &e) {
				}
			}
		}

		virtual void set(T value) {
			std::unique_lock<std::mutex> value_lock(value_mutex);
			this->value = value;
			value_lock.unlock();

			callChangeHandlers();
		}

		virtual T get() {
			std::unique_lock<std::mutex> value_lock(value_mutex);

			return value;
		}

		Json::Value serialize() override {
			Json::Value json_root;
			json_root["value"] = get();

			return json_root;
		}

	private:
		T value;
		std::mutex value_mutex;

		std::list<std::function<void(T)>> change_handlers;
};

template<typename T> class AtomicParameterTemplate : public ParameterTemplate<T> {
	public:
		AtomicParameterTemplate(T default_value) : value(default_value) {
			
		}

		virtual void set(T value) {
			this->value.store(value);

			ParameterTemplate<T>::callChangeHandlers();
		}

		virtual T get() {
			return value.load();
		}

	private:
		std::atomic<T> value;
};

class ParameterDouble : public AtomicParameterTemplate<double> {
	public:
		ParameterDouble(double default_value, double min, double max, double step) : AtomicParameterTemplate(default_value), min(min), max(max), step(step) {
			
		}

		void set(double value) override {
			if(value < min) {
				std::cerr << "Parameter value of " << value << " subceeds minimum of " << min << "! Parameter value will be set to minimum." << std::endl;
				value = min;
			} else if(value > max) {
				std::cerr << "Parameter value of " << value << " exceeds maximum of " << max << "! Parameter value will be set to maximum." << std::endl;
				value = max;
			}

			AtomicParameterTemplate::set(value);
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
			Json::Value json_root = AtomicParameterTemplate::serialize();
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

class ParameterBool : public AtomicParameterTemplate<bool> {
	public:
		ParameterBool(bool default_value) : AtomicParameterTemplate(default_value) {
			
		}

		Json::Value serialize() override {
			Json::Value json_root = AtomicParameterTemplate::serialize();
			json_root["data_type"] = "bool";

			return json_root;
		}
};

class ParameterString : public ParameterTemplate<std::string> {
	public:
		ParameterString(std::string default_value) : ParameterTemplate(default_value) {
			
		}

		Json::Value serialize() override {
			Json::Value json_root = ParameterTemplate::serialize();
			json_root["data_type"] = "string";

			return json_root;
		}
};

class ParameterStringSelect : public ParameterTemplate<std::string> {
	public:
		ParameterStringSelect(std::string default_value, std::list<std::string> options) : ParameterTemplate(default_value), options(options) {
			
		}

		void setOptions(const std::list<std::string> &options) {
			std::unique_lock<std::mutex> options_lock(options_mutex);

			this->options = options;
		}

		void addOption(const std::string &option) {
			std::unique_lock<std::mutex> options_lock(options_mutex);

			options.push_back(option);
		}

		void clearOptions() {
			std::unique_lock<std::mutex> options_lock(options_mutex);

			options.clear();
		}

		Json::Value serializeOptions() {
			std::unique_lock<std::mutex> options_lock(options_mutex);

			Json::Value json_root = Json::arrayValue;

			for(const auto& item : options) {
				json_root.append(item);
			}

			return json_root;
		}

		void addOptionsChangeHandler(std::function<void(Json::Value)> handler) {
			options_change_handlers.push_back(handler);
		}

		void callOptionsChangeHandlers() {
			for(const auto& handler : options_change_handlers) {
				try {
					handler(serializeOptions());
				} catch(const std::bad_function_call &e) {
				}
			}
		}

		Json::Value serialize() override {
			Json::Value json_root = ParameterTemplate::serialize();
			json_root["data_type"] = "string_select";
			json_root["options"] = serializeOptions();

			return json_root;
		}
	private:
		std::list<std::string> options;
		std::mutex options_mutex;

		std::list<std::function<void(Json::Value)>> options_change_handlers;
};

#endif
