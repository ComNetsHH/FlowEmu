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

#ifndef FIXED_DELAY_MODULE_HPP
#define FIXED_DELAY_MODULE_HPP

#include <cstdint>
#include <queue>
#include <utility>
#include <chrono>

#include <boost/asio.hpp>

#include "../Module.hpp"
#include "../../utils/Packet.hpp"

class FixedDelayModule : public Module {
	public:
		FixedDelayModule(boost::asio::io_service &io_service, double delay);
		FixedDelayModule(boost::asio::io_service &io_service) : FixedDelayModule(io_service, 50.0) {};
		~FixedDelayModule();

		const char* getType() const {
			return "fixed_delay";
		}

		void handleDelayChange();

	private:
		ParameterDouble parameter_delay = {0.0, 0.0, std::numeric_limits<double>::quiet_NaN(), 10.0};

		ReceivingPort<std::shared_ptr<Packet>> input_port_lr;
		SendingPort<std::shared_ptr<Packet>> output_port_lr;
		void receiveFromLeftModule(std::shared_ptr<Packet> packet);
		boost::asio::high_resolution_timer timer_lr;
		std::queue<std::pair<std::chrono::high_resolution_clock::time_point, std::shared_ptr<Packet>>> packet_queue_lr;
		void setQueueTimeoutLr();
		void processQueueLr(const boost::system::error_code& error);

		ReceivingPort<std::shared_ptr<Packet>> input_port_rl;
		SendingPort<std::shared_ptr<Packet>> output_port_rl;
		void receiveFromRightModule(std::shared_ptr<Packet> packet);
		boost::asio::high_resolution_timer timer_rl;
		std::queue<std::pair<std::chrono::high_resolution_clock::time_point, std::shared_ptr<Packet>>> packet_queue_rl;
		void setQueueTimeoutRl();
		void processQueueRl(const boost::system::error_code& error);
};

#endif
