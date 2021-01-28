#include <iostream>
#include <string>

#include "modules/ModuleManager.hpp"
#include "modules/delay/FixedDelayModule.hpp"
#include "modules/loss/UncorrelatedLossModule.hpp"
#include "modules/meter/ThroughputMeter.hpp"
#include "modules/null/NullModule.hpp"
#include "modules/rate/FixedIntervalRateModule.hpp"
#include "modules/socket/RawSocket.hpp"

using namespace std;

int main(int argc, const char *argv[]) {
	string interface_source = "in";
	string interface_sink = "out";

	boost::asio::io_service io_service;

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




	while(1) {
		io_service.poll();
	}

	return 0;
}
