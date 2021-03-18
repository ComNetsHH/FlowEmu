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
};

#endif
