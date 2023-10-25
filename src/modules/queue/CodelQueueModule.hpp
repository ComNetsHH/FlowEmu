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


#ifndef CODEL_QUEUE_MODULE_HPP
#define CODEL_QUEUE_MODULE_HPP

#define CONGESTION_EXPERIENCED 3

#include <atomic>
#include <boost/asio.hpp>
#include <chrono>
#include <memory>
#include <queue>
#include <random>

#include "../../utils/Packet.hpp"
#include "../Module.hpp"

class CodelQueueModule : public Module {
	typedef std::chrono::high_resolution_clock::time_point time_point;
	typedef std::chrono::milliseconds ms;

	typedef struct packet_with_timestamp {
		std::shared_ptr<Packet> packet;
		time_point arrival_time;
	} packet_with_timestamp;

	typedef struct dodequeue_result {
		std::shared_ptr<Packet> packet;
		bool ok_to_drop;
	} dodequeue_result;

public:
	CodelQueueModule(boost::asio::io_service &io_service, size_t buffer_size,
	                 size_t target_queue_time);

	const char *getType() const { return "codel_queue"; }

private:
	const ms INTERVAL = ms(100);
	const uint32_t MAXPACKET = 1;

	bool ecn_mode;
	ms target = ms(15);
	time_point first_above_time_ = time_point(ms(0));
	time_point drop_next_ = time_point(ms(0));
	std::chrono::nanoseconds sojourn_time = std::chrono::nanoseconds(0);
	uint32_t count = 0;
	uint32_t lastcount = 0;
	bool dropping_ = false;

	ReceivingPort<std::shared_ptr<Packet>> input_port;
	RespondingPort<std::shared_ptr<Packet>> output_port;

	ParameterBool parameter_ecn_mode = false;
	ParameterDouble parameter_buffer_size = {
	    100, 1, std::numeric_limits<double>::quiet_NaN(), 1};

	ParameterDouble parameter_target_queue_time = {
	    5, 0, std::numeric_limits<double>::quiet_NaN(), 1};

	Statistic statistic_queue_length;
	Statistic statistic_count;
	Statistic statistic_sojourn_time;
	Statistic statistic_packets;
	Statistic statistic_packets_dropped;
	Statistic statistic_packets_marked;

	uint64_t packets;
	uint64_t packets_dropped;
	uint64_t packets_marked;

	std::queue<packet_with_timestamp> packet_queue_timestamp;

	void enqueue(std::shared_ptr<Packet> packet);
	std::shared_ptr<Packet> dequeue();

	time_point control_law(time_point t, uint32_t count);

	dodequeue_result dodequeue(time_point now);

	boost::asio::high_resolution_timer timer_statistics;
	void statistics(const boost::system::error_code &error);
};

#endif
