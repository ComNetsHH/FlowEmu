#include <iostream>
#include <string>

#include "modules/delay/FixedDelayModule.hpp"
#include "modules/loss/UncorrelatedLossModule.hpp"
#include "modules/null/NullModule.hpp"
#include "modules/socket/RawSocket.hpp"

using namespace std;

int main(int argc, const char *argv[]) {
	string interface_source = "in";
	string interface_sink = "out";

	boost::asio::io_service io_service;

	// Sockets
	LeftRawSocket socket_source(io_service, interface_source);
	RightRawSocket socket_sink(io_service, interface_sink);

	// Modules
	NullModule null_module;
	UncorrelatedLossModule loss_module(0.1);
	FixedDelayModule fixed_delay_module(io_service, 10);

	// Connect modules
	socket_source.setRightModule(&null_module);
	null_module.setLeftModule(&socket_source);

	null_module.setRightModule(&loss_module);
	loss_module.setLeftModule(&null_module);

	loss_module.setRightModule(&fixed_delay_module);
	fixed_delay_module.setLeftModule(&loss_module);

	fixed_delay_module.setRightModule(&socket_sink);
	socket_sink.setLeftModule(&fixed_delay_module);

	while(1) {
		io_service.poll();
	}

	return 0;
}
