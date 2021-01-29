#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <csignal>

#include "modules/ModuleManager.hpp"
#include "modules/delay/FixedDelayModule.hpp"
#include "modules/loss/UncorrelatedLossModule.hpp"
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

	// Module manager
	ModuleManager module_manager;

	// Sockets
	LeftRawSocket socket_source(io_service, interface_source);
	RightRawSocket socket_sink(io_service, interface_sink);

	// Modules
	FixedIntervalRateModule fixed_interval_rate_module(io_service);
	NullModule null_module;
	UncorrelatedLossModule loss_module(0.1);
	FixedDelayModule fixed_delay_module(io_service, 10);
	ThroughputMeter throughput_meter_module(io_service);

	// Connect modules
	module_manager.push_back(&socket_source);
	module_manager.push_back(&fixed_interval_rate_module);
	module_manager.push_back(&throughput_meter_module);
	//module_manager.push_back(&null_module);
	module_manager.push_back(&loss_module);
	module_manager.push_back(&fixed_delay_module);
	module_manager.push_back(&socket_sink);

	// Mqtt
	Mqtt mqtt("localhost", 1883, "channel_emulator");

	mqtt.subscribe("set/loss", [&](const string &topic, const string &message) {
		double loss = stod(message);

		loss_module.setLossProbability(loss);
		mqtt.publish("get/loss", to_string(loss), true);
	});

	mqtt.subscribe("set/delay", [&](const string &topic, const string &message) {
		int delay = stoi(message);

		fixed_delay_module.setDelay(delay);
		mqtt.publish("get/delay", delay, true);
	});

	mqtt.subscribe("set/interval", [&](const string &topic, const string &message) {
		uint64_t interval = stoul(message);

		fixed_interval_rate_module.setInterval(chrono::nanoseconds(interval));
		mqtt.publish("get/interval", interval, true);
	});

	mqtt.subscribe("set/buffer_size", [&](const string &topic, const string &message) {
		size_t buffer_size = stoul(message);

		fixed_interval_rate_module.setBufferSize(buffer_size);
		mqtt.publish("get/buffer_size", buffer_size, true);
	});

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
