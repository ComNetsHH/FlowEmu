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

void RawSocket::send(shared_ptr<Packet> packet) {
	std::vector<boost::asio::const_buffer> send_buffers;
	const auto &packet_bytes = packet->getBytes();
	send_buffers.emplace_back(packet_bytes.data(), packet_bytes.size());
	socket.send(send_buffers);
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

// Left Raw Socket
LeftRawSocket::LeftRawSocket(boost::asio::io_service& io_service, std::string ifname) : RawSocket(io_service, ifname) {
}

void LeftRawSocket::receiveFromRightModule(shared_ptr<Packet> packet) {
	send(packet);
}

void LeftRawSocket::handle_receive(const boost::system::error_code& error, size_t bytes_transferred) {
	if(!error || error == boost::asio::error::message_size) {
		passToRightModule(make_shared<Packet>(recv_buffer.data(), bytes_transferred));

		start_receive();
	}
}

// Right Raw Socket
RightRawSocket::RightRawSocket(boost::asio::io_service& io_service, std::string ifname) : RawSocket(io_service, ifname) {
}

void RightRawSocket::receiveFromLeftModule(shared_ptr<Packet> packet) {
	send(packet);
}

void RightRawSocket::handle_receive(const boost::system::error_code& error, size_t bytes_transferred) {
	if(!error || error == boost::asio::error::message_size) {
		passToLeftModule(make_shared<Packet>(recv_buffer.data(), bytes_transferred));

		start_receive();
	}
}
