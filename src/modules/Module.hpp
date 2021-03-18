#ifndef MODULE_HPP
#define MODULE_HPP

#include <string>
#include <map>
#include <list>

#include <json/json.h>

#include "Port.hpp"

class Module {
	public:
		struct PortInfo {
			enum Side {left, right};

			std::string id;
			std::string label;
			Side side;
			Port* port;
		};

		void setName(const std::string& name) {
			this->name = name;
		}

		std::string getName() const {
			return this->name;
		}

		Json::Value serialize() {
			Json::Value json_root;
			json_root["title"] = name;

			Json::Value json_position;
			json_position["x"] = gui_position_x;
			json_position["y"] = gui_position_y;
			json_root["position"] = json_position;

			Json::Value json_size;
			json_size["width"] = gui_width;
			json_size["height"] = gui_width;
			json_root["size"] = json_size;

			Json::Value json_content = Json::arrayValue;

			Json::Value json_flow;
			json_flow["type"] = "flow";

			Json::Value json_ports;

			Json::Value json_ports_left = Json::arrayValue;
			for(auto const& port_info : ports_info_left) {
				Json::Value json_port;
				json_port["id"] = port_info.id;
				json_port["label"] = port_info.label;
				json_ports_left.append(json_port);
			}
			json_ports["left"] = json_ports_left;

			Json::Value json_ports_right = Json::arrayValue;
			for(auto const& port_info : ports_info_right) {
				Json::Value json_port;
				json_port["id"] = port_info.id;
				json_port["label"] = port_info.label;
				json_ports_right.append(json_port);
			}
			json_ports["right"] = json_ports_right;

			json_flow["ports"] = json_ports;

			json_content.append(json_flow);

			json_root["content"] = json_content;

			return json_root;
		}

		PortInfo getPort(std::string id) {
			return ports.at(id);
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

	private:
		std::string name = "UNNAMED MODULE";

		std::map<std::string, PortInfo> ports;
		std::list<PortInfo> ports_info_left;
		std::list<PortInfo> ports_info_right;

		int gui_position_x = 0;
		int gui_position_y = 0;
		int gui_width = 0;
		int gui_height = 0;
};

#endif
