// http://www.boost.org/doc/libs/1_35_0/doc/html/boost_asio/tutorial/tutdaytime6/src.html

#ifndef RAW_SOCKET_HPP
#define RAW_SOCKET_HPP

#include <boost/array.hpp>
#include <boost/asio.hpp>

#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>

#include "../Module.hpp"
#include "../../utils/Packet.hpp"

class RawSocket {
	protected:
		RawSocket(boost::asio::io_service& io_service, std::string ifname);

		void send(std::shared_ptr<Packet> packet);

		void start_receive();
		virtual void handle_receive(const boost::system::error_code& error, size_t bytes_transferred) = 0;

		boost::asio::generic::raw_protocol::socket socket;
		boost::array<uint8_t, 10000> recv_buffer;
};

class LeftRawSocket : public RawSocket, public ModuleHasRight<std::shared_ptr<Packet>> {
	public:
		LeftRawSocket(boost::asio::io_service& io_service, std::string ifname);

		void receiveFromRightModule(std::shared_ptr<Packet> packet) override;
	private:
		virtual void handle_receive(const boost::system::error_code& error, size_t bytes_transferred);
};

class RightRawSocket : public RawSocket, public ModuleHasLeft<std::shared_ptr<Packet>> {
	public:
		RightRawSocket(boost::asio::io_service& io_service, std::string ifname);

		void receiveFromLeftModule(std::shared_ptr<Packet> packet) override;
	private:
		virtual void handle_receive(const boost::system::error_code& error, size_t bytes_transferred);
};

#endif
