/*
 * FlowEmu - Flow-Based Network Emulator
 * Copyright (c) 2021 Institute of Communication Networks (ComNets),
 *                    Hamburg University of Technology (TUHH),
 *                    https://www.tuhh.de/comnets
 * Copyright (c) 2021 Daniel Stolpmann <daniel.stolpmann@tuhh.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

// Reference: http://www.boost.org/doc/libs/1_35_0/doc/html/boost_asio/tutorial/tutdaytime6/src.html
// Reference: https://stackoverflow.com/a/27731722

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
