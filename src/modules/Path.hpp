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
