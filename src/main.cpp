#include <iostream>
#include <memory>
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
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if(vm.count("help")) {
		cout << "ComNets TUHH Channel Emulator" << endl;
		cout << endl;
		cout << desc << endl;
		return 0;
	}

	if(!vm.count("interface-source") || !vm.count("interface-sink")) {
		cout << "No interfaces given!" << endl;
		return 1;
	}

	cout << "Starting emulator..." << endl;

	// MQTT
	Mqtt mqtt(mqtt_broker, mqtt_port, "channel_emulator");

	// Module manager
	ModuleManager module_manager(io_service, mqtt);

	// Sockets
	module_manager.addModule("socket_source", make_shared<RawSocket>(io_service, interface_source, Module::PortInfo::Side::right));
	module_manager.addModule("socket_sink", make_shared<RawSocket>(io_service, interface_sink, Module::PortInfo::Side::left));

	// Load from file
	if(vm.count("graph")) {
		Json::CharReaderBuilder json_reader_builder;
		unique_ptr<Json::CharReader> json_reader(json_reader_builder.newCharReader());

		Json::Value json_root;
		json_reader->parse(graph.c_str(), graph.c_str() + graph.length(), &json_root, nullptr);

		module_manager.deserialize(json_root);
	} else {
		module_manager.loadFromFile("config/graphs/" + graph_file + ".json");
	}

	// MQTT loop
	thread mqtt_thread([&](){
		while(running) {
			mqtt.loop();
		}
	});

	// Loop
	signal(SIGINT, signalHandler);

	cout << "Successfully started emulator!" << endl;
	/* while(running) {
		io_service.poll();
	} */
	io_service.run();

	// Clean up
	cout << "Stopping emulator..." << endl;
	running = false;

	mqtt_thread.join();

	// Save to autosave file
	module_manager.saveToFile("config/graphs/autosave.json");

	return 0;
}
