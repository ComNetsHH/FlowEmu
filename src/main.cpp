#include <iostream>
#include <string>

#include "RawSocket.hpp"

using namespace std;

int main(int argc, const char *argv[]) {
	string interface_source = "in";
	string interface_sink = "out";

	boost::asio::io_service io_service;

	// Sockets
	RawSocket socket_source(io_service, interface_source);
	RawSocket socket_sink(io_service, interface_sink);

	// Handler
	socket_source.set_receive_handler([&](boost::asio::const_buffer packet) {
		//cout << "Source -> Sink: " << boost::asio::buffer_size(packet) << "B" << endl;
		socket_sink.send(packet);
	});

	socket_sink.set_receive_handler([&](boost::asio::const_buffer packet) {
		//cout << "Sink -> Source: " << boost::asio::buffer_size(packet) << "B" << endl;
		socket_source.send(packet);
	});

	while(1) {
		io_service.poll();
	}

	return 0;
}
