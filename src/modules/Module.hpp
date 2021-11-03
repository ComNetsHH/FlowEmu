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

#ifndef MODULE_HPP
#define MODULE_HPP

#include <string>
#include <map>
#include <list>
#include <stdexcept>

#include <json/json.h>

#include "Parameter.hpp"
#include "Port.hpp"
#include "Statistic.hpp"

class Module {
	public:
		struct PortInfo {
			enum Side {left, right};

			std::string id;
			std::string label;
			Side side;
			Port* port;
		};

		struct ParameterInfo {
			std::string id;
			std::string label;
			std::string unit;
			Parameter* parameter;
		};

		struct StatisticInfo {
			std::string id;
			std::string label;
			std::string unit;
			Statistic* statistic;
		};

		virtual const char* getType() const = 0;

		void setName(const std::string& name) {
			this->name = name;
		}

		std::string getName() const {
			return this->name;
		}

		void setRemovable(bool removable) {
			this->removable = removable;
		}

		bool getRemovable() const {
			return this->removable;
		}

		Json::Value serialize() const {
			Json::Value json_root;
			json_root["title"] = name;
			json_root["type"] = getType();
			json_root["removable"] = removable;

			Json::Value json_position;
			json_position["x"] = gui_position_x;
			json_position["y"] = gui_position_y;
			json_root["position"] = json_position;

			Json::Value json_size;
			json_size["width"] = gui_width;
			json_size["height"] = gui_height;
			json_root["size"] = json_size;

			Json::Value json_content = Json::arrayValue;

			Json::Value json_flow;
			json_flow["type"] = "flow";

			Json::Value json_ports;

			Json::Value json_ports_left = Json::arrayValue;
			for(auto const& port_info : ports_info_left) {
				Json::Value json_port;
				json_port["id"] = port_info.id;
				json_port["type"] = port_info.port->getType();
				json_port["label"] = port_info.label;
				json_ports_left.append(json_port);
			}
			json_ports["left"] = json_ports_left;

			Json::Value json_ports_right = Json::arrayValue;
			for(auto const& port_info : ports_info_right) {
				Json::Value json_port;
				json_port["id"] = port_info.id;
				json_port["type"] = port_info.port->getType();
				json_port["label"] = port_info.label;
				json_ports_right.append(json_port);
			}
			json_ports["right"] = json_ports_right;

			json_flow["ports"] = json_ports;

			json_content.append(json_flow);

			for(auto const& entry : parameters) {
				Json::Value json_parameter = entry.second.parameter->serialize();
				json_parameter["type"] = "parameter";
				json_parameter["id"] = entry.second.id;
				json_parameter["label"] = entry.second.label;
				json_parameter["unit"] = entry.second.unit;
				json_content.append(json_parameter);
			}

			for(auto const& entry : statistics) {
				Json::Value json_statistic;
				json_statistic["type"] = "statistic";
				json_statistic["id"] = entry.second.id;
				json_statistic["label"] = entry.second.label;
				json_statistic["unit"] = entry.second.unit;
				json_content.append(json_statistic);
			}

			json_root["content"] = json_content;

			return json_root;
		}

		void deserialize(const Json::Value &json_root) {
			name = json_root.get("title", name).asString();

			if(json_root.isMember("position")) {
				gui_position_x = json_root["position"].get("x", 0).asInt();
				gui_position_y = json_root["position"].get("y", 0).asInt();
			}

			if(json_root.isMember("size")) {
				gui_width = json_root["size"].get("width", 0).asInt();
				gui_height = json_root["size"].get("height", 0).asInt();
			}

			if(json_root.isMember("content")) {
				for(const auto& item : json_root["content"]) {
					if(item.get("type", "").asString() == "parameter" && item.isMember("value")) {
						const std::string id = item.get("id", "").asString();
						const auto parameter = getParameter(id).parameter;
						const Json::Value value = item.get("value", 0);
						
						if(const auto parameter_double = dynamic_cast<ParameterDouble*>(parameter)) {
							const double value_double = value.asDouble();

							parameter_double->set(value_double);
						}
					}
				}
			}
		}

		const std::map<std::string, PortInfo>& getPorts() {
			return ports;
		}

		PortInfo getPort(std::string id) {
			try {
				return ports.at(id);
			} catch(const std::out_of_range &e) {
				throw std::out_of_range("Unknown port ID: " + id);
			}
		}

		const std::map<std::string, ParameterInfo>& getParameters() {
			return parameters;
		}

		ParameterInfo getParameter(std::string id) {
			try {
				return parameters.at(id);
			} catch(const std::out_of_range &e) {
				throw std::out_of_range("Unknown parameter ID: " + id);
			}
		}

		const std::map<std::string, StatisticInfo>& getStatistics() {
			return statistics;
		}

		StatisticInfo getStatistic(std::string id) {
			try {
				return statistics.at(id);
			} catch(const std::out_of_range &e) {
				throw std::out_of_range("Unknown statistic ID: " + id);
			}
		}

	protected:
		void addPort(PortInfo port_info) {
			ports[port_info.id] = port_info;

			switch(port_info.side) {
				case PortInfo::Side::left:
					ports_info_left.push_back(port_info);
					break;
				case PortInfo::Side::right:
					ports_info_right.push_back(port_info);
					break;
			}
		}

		void addParameter(ParameterInfo parameter_info) {
			parameters[parameter_info.id] = parameter_info;
		}

		void addStatistic(StatisticInfo statistic_info) {
			statistics[statistic_info.id] = statistic_info;
		}

	private:
		std::string name = "UNNAMED MODULE";
		bool removable = true;

		std::map<std::string, PortInfo> ports;
		std::list<PortInfo> ports_info_left;
		std::list<PortInfo> ports_info_right;

		std::map<std::string, ParameterInfo> parameters;

		std::map<std::string, StatisticInfo> statistics;

		int gui_position_x = 0;
		int gui_position_y = 0;
		int gui_width = 0;
		int gui_height = 0;
};

#endif
