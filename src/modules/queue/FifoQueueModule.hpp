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

#ifndef FIFO_QUEUE_MODULE_HPP
#define FIFO_QUEUE_MODULE_HPP

#include <atomic>
#include <memory>
#include <queue>

#include <boost/asio.hpp>

#include "../Module.hpp"
#include "../../utils/Packet.hpp"

class FifoQueueModule : public Module {
	public:
		FifoQueueModule(boost::asio::io_service &io_service, size_t buffer_size);

		const char* getType() const {
			return "fifo_queue";
		}

	private:
		ReceivingPort<std::shared_ptr<Packet>> input_port;
		RespondingPort<std::shared_ptr<Packet>> output_port;

		ParameterDouble parameter_buffer_size = {100, 1, std::numeric_limits<double>::quiet_NaN(), 1};

		Statistic statistic_queue_length;

		std::queue<std::shared_ptr<Packet>> packet_queue;

		void enqueue(std::shared_ptr<Packet> packet);
		std::shared_ptr<Packet> dequeue();

		boost::asio::high_resolution_timer timer_statistics;
		void statistics(const boost::system::error_code& error);
};

#endif
