// http://www.boost.org/doc/libs/1_35_0/doc/html/boost_asio/tutorial/tutdaytime6/src.html

#ifndef RAW_SOCKET_HPP
#define RAW_SOCKET_HPP

#include <string>
#include <memory>
#include <cstdint>

#include <boost/array.hpp>
#include <boost/asio.hpp>

#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>

#include "../Module.hpp"
#include "../../utils/Packet.hpp"

class RawSocket : public Module {
	public:
		RawSocket(boost::asio::io_service& io_service, std::string ifname, PortInfo::Side ports_side);
		~RawSocket();

		const char* getType() const {
			return "raw_socket";
		}

	private:
		ReceivingPort<std::shared_ptr<Packet>> input_port;
		SendingPort<std::shared_ptr<Packet>> output_port;

		void send(std::shared_ptr<Packet> packet);

		void startReceive();
		void handleReceive(const boost::system::error_code& error, size_t bytes_transferred);

		boost::asio::generic::raw_protocol::socket socket;
		boost::array<uint8_t, 10000> recv_buffer;
};

#endif
