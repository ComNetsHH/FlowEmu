// http://www.boost.org/doc/libs/1_35_0/doc/html/boost_asio/tutorial/tutdaytime6/src.html

#ifndef RAW_SOCKET_HPP
#define RAW_SOCKET_HPP

#include <functional>

#include <boost/array.hpp>
#include <boost/asio.hpp>

#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>

class RawSocket {
	public:
		RawSocket(boost::asio::io_service& io_service, std::string ifname);
		void send(boost::asio::const_buffer send_buffer);
		void set_receive_handler(std::function<void(boost::asio::const_buffer)> handler);

	private:
		void start_receive();
		void handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred);

		boost::asio::generic::raw_protocol::socket socket;
		boost::array<uint8_t, 10000> recv_buffer;
		std::function<void(boost::asio::const_buffer)> receive_handler;
};

#endif
