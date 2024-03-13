/*
 * FlowEmu - Flow-Based Network Emulator
 * Copyright (c) 2024 Institute of Communication Networks (ComNets),
 *                    Hamburg University of Technology (TUHH),
 *                    https://www.tuhh.de/comnets
 * Copyright (c) 2024 Daniel Stolpmann <daniel.stolpmann@tuhh.de>
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

#ifndef TRACE_LOSS_MODULE_HPP
#define TRACE_LOSS_MODULE_HPP

#include <memory>
#include <mutex>
#include <vector>

#include <boost/asio.hpp>

#include "../Module.hpp"
#include "../../utils/Packet.hpp"

class TraceLossModule : public Module {
	public:
		TraceLossModule(boost::asio::io_service &io_service, const std::string &traces_path, const std::string &trace_filename_lr, const std::string &trace_filename_rl);
		TraceLossModule(boost::asio::io_service &io_service) : TraceLossModule(io_service, "config/traces/loss", "example", "example") {};

		const char* getType() const {
			return "trace_loss";
		}

		void reset();

	private:
		ReceivingPort<std::shared_ptr<Packet>> input_port_lr;
		SendingPort<std::shared_ptr<Packet>> output_port_lr;
		ReceivingPort<std::shared_ptr<Packet>> input_port_rl;
		SendingPort<std::shared_ptr<Packet>> output_port_rl;

		ParameterStringSelect parameter_trace_filename_lr = {"", {}};
		ParameterStringSelect parameter_trace_filename_rl = {"", {}};

		std::vector<bool> trace_lr;
		std::vector<bool>::iterator trace_lr_itr;
		std::mutex trace_lr_mutex;
		void receiveFromLeftModule(std::shared_ptr<Packet> packet);

		std::vector<bool> trace_rl;
		std::vector<bool>::iterator trace_rl_itr;
		std::mutex trace_rl_mutex;
		void receiveFromRightModule(std::shared_ptr<Packet> packet);

		void listTraces(const std::string &path);
		void loadTrace(std::vector<bool> &trace, const std::string &trace_filename);
};

#endif
