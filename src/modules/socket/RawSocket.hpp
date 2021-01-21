// http://www.boost.org/doc/libs/1_35_0/doc/html/boost_asio/tutorial/tutdaytime6/src.html

#ifndef RAW_SOCKET_HPP
#define RAW_SOCKET_HPP

#include <boost/array.hpp>
#include <boost/asio.hpp>

#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>

#include "../Module.hpp"

class RawSocket {
	protected:
		RawSocket(boost::asio::io_service& io_service, std::string ifname);

		void send(boost::asio::const_buffer send_buffer);

		void start_receive();
		virtual void handle_receive(const boost::system::error_code& error, size_t bytes_transferred) = 0;

		boost::asio::generic::raw_protocol::socket socket;
		boost::array<uint8_t, 10000> recv_buffer;
};

class LeftRawSocket : public RawSocket, public ModuleHasRight<boost::asio::const_buffer> {
	public:
		LeftRawSocket(boost::asio::io_service& io_service, std::string ifname);

		void receiveFromRightModule(boost::asio::const_buffer packet) override;
	private:
		virtual void handle_receive(const boost::system::error_code& error, size_t bytes_transferred);
};

class RightRawSocket : public RawSocket, public ModuleHasLeft<boost::asio::const_buffer> {
	public:
		RightRawSocket(boost::asio::io_service& io_service, std::string ifname);

		void receiveFromLeftModule(boost::asio::const_buffer packet) override;
	private:
		virtual void handle_receive(const boost::system::error_code& error, size_t bytes_transferred);
};

#endif
