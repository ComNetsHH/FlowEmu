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

#include <iostream>
#include <memory>
#include <regex>
#include <string>
#include <thread>
#include <atomic>
#include <csignal>

#include <boost/asio.hpp>
#include <boost/program_options.hpp>

#include "modules/ModuleManager.hpp"
#include "modules/socket/RawSocket.hpp"
#include "utils/Mqtt.hpp"

using namespace std;

atomic<bool> running(true);
atomic<bool> mqtt_running(true);
boost::asio::io_service io_service;

void signalHandler(int signum) {
	running = false;
	io_service.stop();
}

namespace po = boost::program_options;

int main(int argc, const char *argv[]) {
	string interface_source;
	string interface_sink;
	string mqtt_broker;
	uint16_t mqtt_port;
	string graph_file;
	string graph;

	po::options_description desc("Available options");
	desc.add_options()
		("help", "Display this help message and exit")
		("interface-source", po::value<string>(&interface_source), "Name of network interface connected to source")
		("interface-sink", po::value<string>(&interface_sink), "Name of network interface connected to sink")
		("mqtt-host", po::value<string>(&mqtt_broker)->default_value("localhost"), "MQTT broker host")
		("mqtt-port", po::value<uint16_t>(&mqtt_port)->default_value(1883), "MQTT broker port")
		("graph-file", po::value<string>(&graph_file)->default_value("autosave"), "Graph file that will be loaded")
		("graph", po::value<string>(&graph), "Graph data in JSON format that will be loaded")
	;

	po::variables_map vm;
	po::parsed_options parsed = po::command_line_parser(argc, argv).options(desc).allow_unregistered().run();
	po::store(parsed, vm);
	po::notify(vm);

	if(vm.count("help")) {
		cout << "FlowEmu" << endl;
		cout << endl;
		cout << desc << endl;
		return 0;
	}

	if(!vm.count("interface-source") || !vm.count("interface-sink")) {
		cout << "No interfaces given!" << endl;
		return 1;
	}

	cout << "Starting FlowEmu..." << endl;

	// MQTT
	Mqtt mqtt(mqtt_broker, mqtt_port, "FlowEmu");

	// Module manager
	ModuleManager module_manager(io_service, mqtt);

	// Sockets
	auto socket_source = make_shared<RawSocket>(io_service, interface_source, Module::PortInfo::Side::right);
	socket_source->setRemovable(false);
	module_manager.addModule("socket_source", socket_source);
	auto socket_sink = make_shared<RawSocket>(io_service, interface_sink, Module::PortInfo::Side::left);
	socket_sink->setRemovable(false);
	module_manager.addModule("socket_sink", socket_sink);

	// Load graph
	if(vm.count("graph")) {
		Json::CharReaderBuilder json_reader_builder;
		unique_ptr<Json::CharReader> json_reader(json_reader_builder.newCharReader());

		Json::Value json_root;
		json_reader->parse(graph.c_str(), graph.c_str() + graph.length(), &json_root, nullptr);

		module_manager.deserialize(json_root);
	} else {
		module_manager.loadFromFile("config/graphs/" + graph_file + ".json");
	}
	
	// Get module parameters from command line
	for(const string& option : po::collect_unrecognized(parsed.options, po::collect_unrecognized_mode::exclude_positional)) {
		regex r("--([\\w-]+)\\.([\\w-]+)=(.*)");
		smatch m;
		if(!(regex_search(option, m, r) && m.size() == 4)) {
			continue;
		}

		string module_id = m.str(1);
		string parameter_id = m.str(2);
		string value = m.str(3);

		try {
			const auto parameter = module_manager.getModule(module_id)->getParameter(parameter_id).parameter;

			if(const auto parameter_double = dynamic_cast<ParameterDouble*>(parameter)) {
				const double value_double = stod(value);

				parameter_double->set(value_double);
			} else if(const auto parameter_bool = dynamic_cast<ParameterBool*>(parameter)) {
				const bool value_bool = (stoi(value) != 0);

				parameter_bool->set(value_bool);
			} else if(const auto parameter_string = dynamic_cast<ParameterString*>(parameter)) {
				parameter_string->set(value);
			} else if(const auto parameter_string_select = dynamic_cast<ParameterStringSelect*>(parameter)) {
				parameter_string_select->set(value);
			}
		} catch(const std::exception &e) {
			cerr << "Error while setting module parameter from command line: " << e.what() << endl;
		}
	}

	// Start MQTT thread
	thread mqtt_thread([&](){
		while(mqtt_running) {
			mqtt.loop();
		}
	});

	// Loop
	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);

	cout << "Successfully started FlowEmu!" << endl;
	/* while(running) {
		io_service.poll();
	} */
	io_service.run();

	// Clean up
	cout << "Stopping FlowEmu..." << endl;

	// Save to autosave file
	module_manager.saveToFile("config/graphs/autosave.json");

	// Remove all modules
	module_manager.clear();

	// Stop MQTT thread
	mqtt_running = false;
	mqtt_thread.join();

	return 0;
}
