#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <csignal>

#include <boost/asio.hpp>

#include "modules/ModuleManager.hpp"
// #include "modules/delay/FixedDelayModule.hpp"
// #include "modules/loss/GilbertElliotLossModule.hpp"
// #include "modules/loss/UncorrelatedLossModule.hpp"
// #include "modules/meter/DelayMeter.hpp"
// #include "modules/meter/ThroughputMeter.hpp"
// #include "modules/null/NullModule.hpp"
// #ifdef MACHINE_LEARNING
// #include "modules/queue/DQLQueueModule.hpp"
// #endif
// #include "modules/rate/BitrateRateModule.hpp"
// #include "modules/rate/FixedIntervalRateModule.hpp"
// #include "modules/rate/TraceRateModule.hpp"
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
	if(argc > 2) {
		interface_source = argv[1];
		interface_sink = argv[2];
	}

	cout << "Starting emulator..." << endl;

	// MQTT
	Mqtt mqtt("localhost", 1883, "channel_emulator");

	// Module manager
	ModuleManager module_manager(mqtt);

	// Sockets
	module_manager.addModule("socket_source", make_shared<RawSocket>(io_service, interface_source, Module::PortInfo::Side::right));
	module_manager.addModule("socket_sink", make_shared<RawSocket>(io_service, interface_sink, Module::PortInfo::Side::left));

	// Modules
	//BitrateRateModule bitrate_rate_module(io_service, mqtt, 1000000, 100);
	//FixedIntervalRateModule fixed_interval_rate_module(io_service, mqtt, chrono::milliseconds(1), 100);
	//TraceRateModule trace_rate_module(io_service, mqtt, "/config/traces/trace.down", "/config/traces/trace.up", 100);
	//#ifdef MACHINE_LEARNING
	//DQLQueueModule dql_queue_module(io_service, mqtt, chrono::milliseconds(1));
	//#endif
	//NullModule null_module;
	//UncorrelatedLossModule uncorrelated_loss_module(mqtt, 0);
	//GilbertElliotLossModule gilbert_elliot_loss_module(io_service, mqtt, 1.0/10000, 1.0/100, 0, 0.1);
	//FixedDelayModule fixed_delay_module(io_service, mqtt, 0);
	//DelayMeter delay_meter_module(io_service, mqtt);
	//ThroughputMeter throughput_meter_module_left(io_service, mqtt, "throughput_meter_module_left");
	//ThroughputMeter throughput_meter_module_right(io_service, mqtt, "throughput_meter_module_right");

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
