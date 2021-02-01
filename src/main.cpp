#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <csignal>

#include "modules/ModuleManager.hpp"
#include "modules/delay/FixedDelayModule.hpp"
#include "modules/loss/UncorrelatedLossModule.hpp"
#include "modules/meter/DelayMeter.hpp"
#include "modules/meter/ThroughputMeter.hpp"
#include "modules/null/NullModule.hpp"
#include "modules/rate/FixedIntervalRateModule.hpp"
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
	string interface_source = "in";
	string interface_sink = "out";

	// MQTT
	Mqtt mqtt("localhost", 1883, "channel_emulator");

	// Module manager
	ModuleManager module_manager;

	// Sockets
	LeftRawSocket socket_source(io_service, interface_source);
	RightRawSocket socket_sink(io_service, interface_sink);

	// Modules
	FixedIntervalRateModule fixed_interval_rate_module(io_service, mqtt, chrono::milliseconds(1), 100);
	//NullModule null_module;
	UncorrelatedLossModule loss_module(mqtt, 0);
	FixedDelayModule fixed_delay_module(io_service, mqtt, 0);
	DelayMeter delay_meter_module(io_service, mqtt);
	ThroughputMeter throughput_meter_module(io_service, mqtt);

	// Connect modules
	module_manager.push_back(&socket_source);
	module_manager.push_back(&throughput_meter_module);
	module_manager.push_back(&fixed_interval_rate_module);
	//module_manager.push_back(&null_module);
	module_manager.push_back(&loss_module);
	module_manager.push_back(&fixed_delay_module);
	module_manager.push_back(&delay_meter_module);
	module_manager.push_back(&socket_sink);

	// MQTT loop
	thread mqtt_thread([&](){
		while(running) {
			mqtt.loop();
		}
	});

	// Loop
	signal(SIGINT, signalHandler);

	/* while(running) {
		io_service.poll();
	} */
	io_service.run();

	// Clean up
	mqtt_thread.join();
	return 0;
}