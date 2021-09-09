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

#ifndef BITRATE_RATE_MODULE_HPP
#define BITRATE_RATE_MODULE_HPP

#include <cstdint>
#include <queue>
#include <utility>
#include <atomic>
#include <chrono>

#include <boost/asio.hpp>

#include "../Module.hpp"
#include "../../utils/Packet.hpp"

class BitrateRateModule : public Module {
	public:
		BitrateRateModule(boost::asio::io_service &io_service, uint64_t bitrate);
		~BitrateRateModule();

		const char* getType() const {
			return "bitrate_rate";
		}

	private:
		RequestingPort<std::shared_ptr<Packet>> input_port;
		SendingPort<std::shared_ptr<Packet>> output_port;

		Parameter parameter_bitrate = {1000000, 0, std::numeric_limits<double>::quiet_NaN(), 1000};

		boost::asio::high_resolution_timer timer;
		std::shared_ptr<Packet> current_transmission = nullptr;
		void process(const boost::system::error_code& error);
};

#endif
