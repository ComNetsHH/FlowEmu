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

#ifndef FIXED_INTERVAL_RATE_MODULE_HPP
#define FIXED_INTERVAL_RATE_MODULE_HPP

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>

#include <boost/asio.hpp>

#include "../Module.hpp"
#include "../../utils/Packet.hpp"

class FixedIntervalRateModule : public Module {
	public:
		FixedIntervalRateModule(boost::asio::io_service &io_service, std::chrono::high_resolution_clock::duration interval);
		~FixedIntervalRateModule();

		const char* getType() const {
			return "fixed_interval_rate";
		}

	private:
		RequestingPort<std::shared_ptr<Packet>> input_port_lr;
		SendingPort<std::shared_ptr<Packet>> output_port_lr;
		RequestingPort<std::shared_ptr<Packet>> input_port_rl;
		SendingPort<std::shared_ptr<Packet>> output_port_rl;

		ParameterDouble parameter_interval = {1, 0, std::numeric_limits<double>::quiet_NaN(), 1};
		ParameterDouble parameter_rate = {1000, 0, std::numeric_limits<double>::quiet_NaN(), 1};

		boost::asio::high_resolution_timer timer;
		void process(const boost::system::error_code& error);
};

#endif
