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


#include "RedQueueModule.hpp"

#include <boost/bind.hpp>
#include <cmath>

using namespace std;

RedQueueModule::RedQueueModule(boost::asio::io_service &io_service,
                               size_t buffer_size, double queue_weight,
                               uint32_t min_threshold_input,
                               uint32_t max_threshold_input, double max_p,
                               double transmission_time, uint32_t seed)
    : timer_statistics(io_service) {
	setName("Red Queue");
	addPort({"in", "In", PortInfo::Side::left, &input_port});
	addPort({"out", "Out", PortInfo::Side::right, &output_port});
	addParameter({"buffer_size", "Buffer", "packets", &parameter_buffer_size});
	addParameter({"ecn_mode", "ECN Mode", "", &parameter_ecn_mode});
	addParameter({"queue_weight", "Weight", "", &parameter_queue_weight});
	addParameter(
	    {"min_threshold", "Min_th", "packets", &parameter_min_threshold});
	addParameter(
	    {"max_threshold", "Max_th", "packets", &parameter_max_threshold});
	addParameter({"max_p", "Max_p", "", &parameter_max_p});
	addParameter({"transmission_time", "Avg. packet transmission", "ms",
	              &parameter_transmission_time});
	addParameter({"seed", "Seed", "", &parameter_seed});
	addStatistic({"drop_prob", "Drop Prob", "", &statistic_drop_prob});
	addStatistic({"avg", "Average queue length", "packets", &statistic_avg});
	addStatistic({"count", "Count not dropped", "packets", &statistic_count});
	addStatistic({"queue_length", "Queue", "packets", &statistic_queue_length});
	addStatistic({"packets", "Packets Received", "", &statistic_packets});
	addStatistic({"packets_dropped", "Packets Droppped", "", &statistic_packets_dropped});
	addStatistic({"packets_marked", "Packets Marked", "", &statistic_packets_marked});

	input_port.setReceiveHandler(
	    bind(&RedQueueModule::receivePacket, this, placeholders::_1));
	output_port.setRequestHandler(bind(&RedQueueModule::dequeue, this));

	min_threshold = min_threshold_input;
	max_threshold = max_threshold_input;

	parameter_buffer_size.set(buffer_size);
	parameter_queue_weight.set(queue_weight);
	parameter_min_threshold.addChangeHandler([&](uint64_t value) {
		if (value >= max_threshold) {
			min_threshold = max_threshold - 1;
			parameter_min_threshold.set(min_threshold);
		} else {
			min_threshold = value;
		}
	});
	parameter_min_threshold.callChangeHandlers();
	parameter_max_threshold.addChangeHandler([&](uint64_t value) {
		if (value <= min_threshold) {
			max_threshold = min_threshold + 1;
			parameter_max_threshold.set(max_threshold);
		} else {
			max_threshold = value;
		}
	});
	parameter_max_threshold.callChangeHandlers();
	parameter_max_p.set(max_p);
	parameter_transmission_time.set(transmission_time);

	parameter_ecn_mode.addChangeHandler([&](bool value) { ecn_mode = value; });
	parameter_ecn_mode.callChangeHandlers();

	distribution.reset(new uniform_real_distribution(0.0, 1.0));

	parameter_seed.addChangeHandler([&](double value) {
		generator.seed(value);
	});
	parameter_seed.set(seed);

	q_time = chrono::high_resolution_clock::now();

	packets = 0;
	packets_dropped = 0;
	packets_marked = 0;

	// Start statistics timer
	timer_statistics.expires_from_now(chrono::milliseconds(0));
	timer_statistics.async_wait(boost::bind(&RedQueueModule::statistics, this,
	                                        boost::asio::placeholders::error));
}

void RedQueueModule::receivePacket(shared_ptr<Packet> packet) {
	bool packet_marked = false;
	uint8_t ecn = 0;
	if (ecn_mode) {
		ecn = packet->getECN();
	}
	packets++;

	// Recalculate Average
	if (packet_queue.size() > 0) {
		avg = (1 - parameter_queue_weight.get()) * avg +
		      parameter_queue_weight.get() * packet_queue.size();
	} else {
		auto m = chrono::high_resolution_clock::now() - q_time;
		avg = pow((1 - parameter_queue_weight.get()),
		          (chrono::duration_cast<chrono::milliseconds>(m).count() /
		           parameter_transmission_time.get())) *
		      avg;
	}

	// calculate marking probability p_a
	if ((min_threshold <= avg) && (avg < max_threshold)) {
		count++;
		double p_b = parameter_max_p.get() * (avg - min_threshold) /
		             (max_threshold - min_threshold);
		p_a = p_b / (1 - count * p_b);
		if (p_a < 0.0) {
			p_a = 0.0;
		}

		// mark packet with probability p_a
		if ((*distribution)(generator) <= p_a) {
			packet_marked = true;
			count = 0;
		}
	} else if (max_threshold < avg) {
		packet_marked = true;
		count = 0;
	} else {
		count = -1;
	}

	if (!packet_marked) {
		if (packet_queue.size() >= parameter_buffer_size.get()) {
			packets_dropped++;
			return;
		}
		packet_queue.emplace(packet);

		output_port.notify();
	} else {
		if(ecn_mode && (ecn == ECT0 || ecn == ECT1)) {
			packets_marked++;
			packet->setECN(CONGESTION_EXPERIENCED);
			packet_queue.emplace(packet);

			output_port.notify();
		} else {
			packets_dropped++;
		}
	}
	return;
}

std::shared_ptr<Packet> RedQueueModule::dequeue() {
	shared_ptr<Packet> packet;
	if (!packet_queue.empty()) {
		packet = packet_queue.front();
		packet_queue.pop();

		if (packet_queue.empty()) {
			q_time = chrono::high_resolution_clock::now();
		}
	}
	return packet;
}

void RedQueueModule::statistics(const boost::system::error_code &error) {
	if (error == boost::asio::error::operation_aborted) {
		return;
	}
	statistic_drop_prob.set(p_a);
	statistic_avg.set(avg);
	statistic_count.set(count);
	statistic_queue_length.set(packet_queue.size());
	statistic_packets.set(packets);
	statistic_packets_dropped.set(packets_dropped);
	statistic_packets_marked.set(packets_marked);

	timer_statistics.expires_at(timer_statistics.expiry() +
	                            chrono::milliseconds(100));
	timer_statistics.async_wait(boost::bind(&RedQueueModule::statistics, this,
	                                        boost::asio::placeholders::error));
}
