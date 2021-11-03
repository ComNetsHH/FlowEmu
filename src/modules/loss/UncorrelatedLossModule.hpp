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

#ifndef UNCORRELATED_LOSS_MODULE_HPP
#define UNCORRELATED_LOSS_MODULE_HPP

#include <cstdint>
#include <random>
#include <atomic>
#include <memory>

#include <boost/asio.hpp>

#include "../Module.hpp"
#include "../../utils/Packet.hpp"

class UncorrelatedLossModule : public Module {
	public:
		UncorrelatedLossModule(double loss, uint32_t seed = 1);

		const char* getType() const {
			return "uncorrelated_loss";
		}

	private:
		ReceivingPort<std::shared_ptr<Packet>> input_port_lr;
		SendingPort<std::shared_ptr<Packet>> output_port_lr;
		ReceivingPort<std::shared_ptr<Packet>> input_port_rl;
		SendingPort<std::shared_ptr<Packet>> output_port_rl;

		ParameterDouble parameter_loss = {10, 0, 100, 1};
		ParameterDouble parameter_seed = {1, 0, std::numeric_limits<double>::quiet_NaN(), 1};

		std::mt19937 generator_loss;
		std::unique_ptr<std::bernoulli_distribution> distribution;

		void receiveFromLeftModule(std::shared_ptr<Packet> packet);
		void receiveFromRightModule(std::shared_ptr<Packet> packet);
};

#endif
