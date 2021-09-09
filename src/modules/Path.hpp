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

#ifndef PATH_HPP
#define PATH_HPP

#include <string>

#include "Module.hpp"

struct Path {
	std::string from_node_id;
	std::string from_port_id;
	std::string to_node_id;
	std::string to_port_id;

	Module::PortInfo from_port_info;
	Module::PortInfo to_port_info;

	Json::Value serialize() const {
		Json::Value json_root;

		Json::Value json_from;
		json_from["node"] = from_node_id;
		json_from["port"] = from_port_id;
		json_root["from"] = json_from;

		Json::Value json_to;
		json_to["node"] = to_node_id;
		json_to["port"] = to_port_id;
		json_root["to"] = json_to;

		return json_root;
	}

	void deserialize(const Json::Value &json_root) {
		from_node_id = json_root["from"].get("node", "").asString();
		from_port_id = json_root["from"].get("port", "").asString();
		to_node_id = json_root["to"].get("node", "").asString();
		to_port_id = json_root["to"].get("port", "").asString();
	}
};

#endif
