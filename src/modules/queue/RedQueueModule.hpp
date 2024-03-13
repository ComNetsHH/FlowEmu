/*
 * FlowEmu - Flow-Based Network Emulator
 * Copyright (c) 2022 Institute of Communication Networks (ComNets),
 *                    Hamburg University of Technology (TUHH),
 *                    https://www.tuhh.de/comnets
 * Copyright (c) 2022 Jesper Dell Missier <jesper.dell@tuhh.de>
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


#ifndef RED_QUEUE_MODULE_HPP
#define RED_QUEUE_MODULE_HPP

#define CONGESTION_EXPERIENCED 3
#define ECT0 2
#define ECT1 1

#include <atomic>
#include <boost/asio.hpp>
#include <chrono>
#include <memory>
#include <queue>
#include <random>

#include "../../utils/Packet.hpp"
#include "../Module.hpp"

class RedQueueModule : public Module {
public:
	RedQueueModule(boost::asio::io_service &io_service, size_t buffer_size,
	               double queue_weight, uint32_t min_threshold_input,
	               uint32_t max_threshold_input, double max_p,
	               double transmission_time, uint32_t seed = 1);
	RedQueueModule(boost::asio::io_service &io_service) : RedQueueModule(io_service, 100, 0.002, 15, 45, 0.1, 1.0) {};

	const char *getType() const { return "red_queue"; }

private:
	ReceivingPort<std::shared_ptr<Packet>> input_port;
	RespondingPort<std::shared_ptr<Packet>> output_port;

	ParameterDouble parameter_buffer_size = {
	    100, 1, std::numeric_limits<double>::quiet_NaN(), 1};
	ParameterBool parameter_ecn_mode = false;
	ParameterDouble parameter_queue_weight = {0.002, 0.001, 0.005, 0.001};
	ParameterDouble parameter_min_threshold = {15, 1, 100, 1};
	ParameterDouble parameter_max_threshold = {45, 1, 100, 1};
	ParameterDouble parameter_max_p = {0.1, 0.1, 0.1, 0.1};
	ParameterDouble parameter_transmission_time = {
	    1.0, 0.001, std::numeric_limits<double>::quiet_NaN(), 0.01};
	ParameterDouble parameter_seed = {1, 0, std::numeric_limits<double>::quiet_NaN(), 1};

	bool ecn_mode;

	uint64_t min_threshold;
	uint64_t max_threshold;

	uint64_t packets;
	uint64_t packets_dropped;
	uint64_t packets_marked;

	Statistic statistic_drop_prob;
	Statistic statistic_avg;
	Statistic statistic_count;
	Statistic statistic_queue_length;
	Statistic statistic_packets;
	Statistic statistic_packets_dropped;
	Statistic statistic_packets_marked;

	double avg = 0;
	int32_t count = -1;
	std::chrono::time_point<std::chrono::high_resolution_clock> q_time;
	double p_a = 0;

	std::mt19937 generator;
	std::unique_ptr<std::uniform_real_distribution<double>> distribution;

	std::queue<std::shared_ptr<Packet>> packet_queue;

	void receivePacket(std::shared_ptr<Packet> packet);
	std::shared_ptr<Packet> dequeue();

	boost::asio::high_resolution_timer timer_statistics;
	void statistics(const boost::system::error_code &error);
};

#endif
