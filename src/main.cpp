#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <csignal>

#include <boost/asio.hpp>

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

int main(int argc, const char *argv[]) {
	string mqtt_broker = "localhost";
	string interface_source = "in";
	string interface_sink = "out";

	if(argc > 1) {
		mqtt_broker = argv[1];
	}
	if(argc > 3) {
		interface_source = argv[2];
		interface_sink = argv[3];
	}

	cout << "Starting emulator..." << endl;

	// MQTT
	Mqtt mqtt(mqtt_broker, 1883, "channel_emulator");

	// Module manager
	ModuleManager module_manager(io_service, mqtt);

	// Sockets
	module_manager.addModule("socket_source", make_shared<RawSocket>(io_service, interface_source, Module::PortInfo::Side::right));
	module_manager.addModule("socket_sink", make_shared<RawSocket>(io_service, interface_sink, Module::PortInfo::Side::left));

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

	return 0;
}
