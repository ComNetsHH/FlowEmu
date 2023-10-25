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


#ifndef PI2_QUEUE_MODULE_HPP
#define PI2_QUEUE_MODULE_HPP

#define CONGESTION_EXPERIENCED 3
#define ECT0 2
#define ECT1 1

#define MAX_PROB 0.25

#include <atomic>
#include <boost/asio.hpp>
#include <memory>
#include <queue>
#include <random>

#include "../../utils/Packet.hpp"
#include "../Module.hpp"

class Pi2QueueModule : public Module {
	typedef std::chrono::high_resolution_clock::time_point time_point;
	typedef std::chrono::milliseconds ms;

	typedef struct packet_with_timestamp {
		std::shared_ptr<Packet> packet;
		time_point arrival_time;
	} packet_with_timestamp;

public:
	Pi2QueueModule(boost::asio::io_service &io_service, size_t buffer_size,
	               uint64_t qdelay_ref_input, uint64_t rtt_max_input, uint32_t seed = 1);

	const char *getType() const { return "pi2_queue"; }

private:
	const double mark_ecnth = 0.1;

	ReceivingPort<std::shared_ptr<Packet>> input_port;
	RespondingPort<std::shared_ptr<Packet>> output_port;

	ParameterBool parameter_ecn_mode = false;
	ParameterDouble parameter_buffer_size = {
	    100, 1, std::numeric_limits<double>::quiet_NaN(), 1};
	ParameterDouble parameter_qdelay_ref = {
	    15, 0, std::numeric_limits<double>::quiet_NaN(), 1};
	ParameterDouble parameter_rtt_max = {100, 1,
	                                     std::numeric_limits<double>::quiet_NaN(), 1};
	ParameterDouble parameter_seed = {1, 0, std::numeric_limits<double>::quiet_NaN(), 1};

	bool ecn_mode;
	ms qdelay_ref;
	ms rtt_max;

	ms t_update;
	double alpha;
	double beta;
	double pseudo_p;

	ms qdelay_current;
	double drop_prob;
	ms qdelay_old;

	uint64_t packets;
	uint64_t packets_dropped;
	uint64_t packets_marked;

	std::mt19937 generator;
	std::unique_ptr<std::uniform_real_distribution<double>> distribution;

	Statistic statistic_queue_length;
	Statistic statistic_drop_prob;
	Statistic statistic_pseudo_p;
	Statistic statistic_queue_delay;
	Statistic statistic_packets;
	Statistic statistic_packets_dropped;
	Statistic statistic_packets_marked;

	std::queue<packet_with_timestamp> packet_queue;

	void enqueue(std::shared_ptr<Packet> packet);
	std::shared_ptr<Packet> dequeue();

	void update_alpha_beta();
	bool drop_early(uint8_t ecn);
	boost::asio::high_resolution_timer timer_probability_update;
	void calculate_drop_prob(const boost::system::error_code &error);

	boost::asio::high_resolution_timer timer_statistics;
	void statistics(const boost::system::error_code &error);
};

#endif
