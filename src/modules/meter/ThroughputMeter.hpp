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

#ifndef THROUGHPUT_METER_MODULE_HPP
#define THROUGHPUT_METER_MODULE_HPP

#include <queue>
#include <utility>
#include <memory>
#include <string>

#include <boost/asio.hpp>

#include "../Module.hpp"
#include "../../utils/Packet.hpp"

class ThroughputMeter : public Module {
	public:
		ThroughputMeter(boost::asio::io_service &io_service);
		~ThroughputMeter();

		const char* getType() const {
			return "throughput_meter";
		}

	private:
		ReceivingPort<std::shared_ptr<Packet>> input_port;
		SendingPort<std::shared_ptr<Packet>> output_port;

		Statistic statistic_bytes_per_second;
		Statistic statistic_packets_per_second;

		void receive(std::shared_ptr<Packet> packet);

		boost::asio::high_resolution_timer timer;
		uint64_t bytes_sum = 0;
		std::queue<std::pair<std::chrono::high_resolution_clock::time_point, uint64_t>> bytes;
		void process(const boost::system::error_code& error);
};

#endif
