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

//
// This module is work in progress and will not perform any meaningful actions!
//

// Inspired by: M. Bachl, J. Fabini and T. Zseby, "LFQ: Online Learning of Per-flow Queuing Policies using Deep Reinforcement Learning," 2020 IEEE 45th Conference on Local Computer Networks (LCN), 2020, pp. 417-420, doi: 10.1109/LCN48667.2020.9314771

#ifndef DQL_QUEUE_MODULE_HPP
#define DQL_QUEUE_MODULE_HPP

#include <vector>
#include <cstdint>
#include <queue>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>

#include <boost/asio.hpp>

#include "../Module.hpp"
#include "../../ml/DeepQLearning.hpp"
#include "../../utils/Packet.hpp"

class DQLQueueModule : public Module {
	public:
		DQLQueueModule(boost::asio::io_service &io_service, size_t buffer_size, double epsilon);
		~DQLQueueModule();

		const char* getType() const {
			return "dql_queue";
		}

	private:
		ReceivingPort<std::shared_ptr<Packet>> input_port;
		RespondingPort<std::shared_ptr<Packet>> output_port;

		ParameterDouble parameter_buffer_size = {100, 0, std::numeric_limits<double>::quiet_NaN(), 1};
		ParameterDouble parameter_epsilon = {0.001, 0, 1, 0.001};

		Statistic statistic_queue_length;

		DeepQLearning deep_q_learning;

		tensorflow::Tensor observation_tf;
		tensorflow::Tensor observation_new_tf;
		int8_t reward;

		std::unique_ptr<std::thread> training_thread;
		std::atomic<bool> running;

		std::queue<std::shared_ptr<Packet>> packet_queue;

		void enqueue(std::shared_ptr<Packet> packet);
		std::shared_ptr<Packet> dequeue();

		boost::asio::high_resolution_timer timer_statistics;
		void statistics(const boost::system::error_code& error);
};

#endif
