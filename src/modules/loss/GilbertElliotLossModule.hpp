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

#ifndef GILBERT_ELLIOT_LOSS_MODULE_HPP
#define GILBERT_ELLIOT_LOSS_MODULE_HPP

#include <cstdint>
#include <random>
#include <atomic>
#include <memory>

#include <boost/asio.hpp>

#include "../Module.hpp"
#include "../../utils/Packet.hpp"

class GilbertElliotLossModule : public Module {
	public:
		GilbertElliotLossModule(boost::asio::io_service &io_service, double p01, double p10, double e0 = 0, double e1 = 100, uint32_t seed_transition = 1, uint32_t seed_loss = 1);
		~GilbertElliotLossModule();

		const char* getType() const {
			return "gilbert_elliot_loss";
		}

		void reset();

	private:
		ReceivingPort<std::shared_ptr<Packet>> input_port_lr;
		SendingPort<std::shared_ptr<Packet>> output_port_lr;
		ReceivingPort<std::shared_ptr<Packet>> input_port_rl;
		SendingPort<std::shared_ptr<Packet>> output_port_rl;

		Parameter parameter_p01 = {0.001, 0, std::numeric_limits<double>::quiet_NaN(), 0.001};
		Parameter parameter_p10 = {0.001, 0, std::numeric_limits<double>::quiet_NaN(), 0.001};
		Parameter parameter_e0 = {0, 0, 100, 1};
		Parameter parameter_e1 = {100, 0, 100, 1};
		Parameter parameter_seed_transition = {1, 0, std::numeric_limits<double>::quiet_NaN(), 1};
		Parameter parameter_seed_loss = {1, 0, std::numeric_limits<double>::quiet_NaN(), 1};
		Statistic statistic_state;

		std::mt19937 generator_transition;
		std::mt19937 generator_loss;
		std::unique_ptr<std::exponential_distribution<double>> distribution_p01;
		std::unique_ptr<std::exponential_distribution<double>> distribution_p10;
		std::unique_ptr<std::bernoulli_distribution> distribution_e0;
		std::unique_ptr<std::bernoulli_distribution> distribution_e1;

		void receiveFromLeftModule(std::shared_ptr<Packet> packet);
		void receiveFromRightModule(std::shared_ptr<Packet> packet);

		std::atomic<bool> state;
		boost::asio::high_resolution_timer timer_transition;
		void transition(const boost::system::error_code& error);
		bool isLost();
};

#endif
