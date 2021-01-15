// https://stackoverflow.com/a/27731722

#include "RawSocket.hpp"

#include <boost/bind.hpp>

using namespace std;

RawSocket::RawSocket(boost::asio::io_service& io_service, std::string ifname) : socket(io_service, boost::asio::generic::raw_protocol(PF_PACKET, SOCK_RAW)) {
	sockaddr_ll sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sll_family = PF_PACKET;
	sockaddr.sll_protocol = htons(ETH_P_ALL);
	sockaddr.sll_ifindex = if_nametoindex(ifname.c_str());
	sockaddr.sll_hatype = 1;

	socket.bind(boost::asio::generic::basic_endpoint<boost::asio::generic::raw_protocol>(&sockaddr, sizeof(sockaddr)));

	start_receive();
}

void RawSocket::send(boost::asio::const_buffer send_buffer) {
	std::vector<boost::asio::const_buffer> send_buffers;
	send_buffers.push_back(send_buffer);
	socket.send(send_buffers);
}

void RawSocket::set_receive_handler(function<void(boost::asio::const_buffer)> handler) {
	receive_handler = handler;
}

void RawSocket::start_receive() {
	socket.async_receive(boost::asio::buffer(recv_buffer),
	                     0,
	                     boost::bind(&RawSocket::handle_receive,
	                                 this,
	                                 boost::asio::placeholders::error,
	                                 boost::asio::placeholders::bytes_transferred
	                     )
	);
}

void RawSocket::handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred) {
	if(!error || error == boost::asio::error::message_size) {
		try {
			receive_handler(boost::asio::buffer(recv_buffer, bytes_transferred));
		} catch(bad_function_call e) {
		}

		start_receive();
	}
}
