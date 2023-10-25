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

#include "Pi2QueueModule.hpp"

#include <boost/bind.hpp>

using namespace std;

Pi2QueueModule::Pi2QueueModule(boost::asio::io_service &io_service,
                               size_t buffer_size,
                               uint64_t qdelay_ref_input,
                               uint64_t rtt_max_input, uint32_t seed)
    : timer_statistics(io_service), timer_probability_update(io_service) {
	setName("PI2 Queue");
	addPort({"in", "In", PortInfo::Side::left, &input_port});
	addPort({"out", "Out", PortInfo::Side::right, &output_port});
	addParameter({"ecn_mode", "ECN Mode", "", &parameter_ecn_mode});
	addParameter({"buffer_size", "Buffer", "packets", &parameter_buffer_size});
	addParameter({"qdelay_ref", "Target QDelay", "ms", &parameter_qdelay_ref});
	addParameter({"rtt_max", "Max Expected RTT", "ms", &parameter_rtt_max});
	addParameter({"seed", "Seed", "", &parameter_seed});
	addStatistic({"queue_length", "Queue", "packets", &statistic_queue_length});
	addStatistic({"drop_prob", "Drop Prob", "", &statistic_drop_prob});
	addStatistic({"pseudo_p", "Pseudo P", "", &statistic_pseudo_p});
	addStatistic({"queue_delay", "Queue Delay", "ms", &statistic_queue_delay});
	addStatistic({"packets", "Packets Received", "", &statistic_packets});
	addStatistic(
	    {"packets_dropped", "Packets Droppped", "", &statistic_packets_dropped});
	addStatistic(
	    {"packets_marked", "Packets Marked", "", &statistic_packets_marked});

	input_port.setReceiveHandler(
	    bind(&Pi2QueueModule::enqueue, this, placeholders::_1));
	output_port.setRequestHandler(bind(&Pi2QueueModule::dequeue, this));

	parameter_buffer_size.set(buffer_size);

	parameter_ecn_mode.addChangeHandler([&](bool value) { ecn_mode = value; });
	parameter_ecn_mode.callChangeHandlers();

	parameter_rtt_max.addChangeHandler([&](uint64_t value) {
    rtt_max = ms(value);
    update_alpha_beta(); });
	parameter_rtt_max.callChangeHandlers();

	parameter_qdelay_ref.addChangeHandler([&](uint64_t value) {
    qdelay_ref = ms(value);
    update_alpha_beta(); });
	parameter_qdelay_ref.callChangeHandlers();

	parameter_seed.addChangeHandler([&](double value) {
		generator.seed(value);
	});
	parameter_seed.set(seed);
	distribution.reset(new uniform_real_distribution(0.0, 1.0));

		
	pseudo_p = 0;
	drop_prob = 0;
	qdelay_current = ms(0);
	qdelay_old = ms(0);

	packets = 0;
	packets_dropped = 0;
	packets_marked = 0;

	// Start timer to update probability periodically
	timer_probability_update.expires_from_now(ms(0));
	timer_probability_update.async_wait(
	    boost::bind(&Pi2QueueModule::calculate_drop_prob,
	                this,
	                boost::asio::placeholders::error));

	// Start statistics timer
	timer_statistics.expires_from_now(ms(0));
	timer_statistics.async_wait(boost::bind(
	    &Pi2QueueModule::statistics, this, boost::asio::placeholders::error));
}

void Pi2QueueModule::update_alpha_beta() {
	t_update = min(qdelay_ref, rtt_max / 3);
	alpha = 0.1 * (static_cast<double>(t_update.count()) / 1000 /
	               (static_cast<double>(rtt_max.count()) / 1000 *
	                static_cast<double>(rtt_max.count()) / 1000));
	beta = 0.3 / (static_cast<double>(rtt_max.count()) / 1000);
}

void Pi2QueueModule::enqueue(shared_ptr<Packet> packet) {
	uint8_t ecn = 0;
	if (ecn_mode) {
		ecn = packet->getECN();
	}
	packets++;

	if (drop_early(ecn)) {
		if ((ecn == ECT0) || (ecn == ECT1)) {
			packet->setECN(CONGESTION_EXPERIENCED);
			packet->updateIPv4HeaderChecksum();
			packets_marked++;
		} else {
			packets_dropped++;
			return;
		}
	}
	if (packet_queue.size() >= parameter_buffer_size.get()) {
		if (packet->getECN() == CONGESTION_EXPERIENCED) {
			packets_marked--;
		}
		packets_dropped++;
		return;
	}
	packet_with_timestamp packet_timestamped = {
	    packet, chrono::high_resolution_clock::now()};
	packet_queue.emplace(packet_timestamped);
	output_port.notify();
}

std::shared_ptr<Packet>
Pi2QueueModule::dequeue() {
	packet_with_timestamp packet_timestamped;
	shared_ptr<Packet> packet;

	if (!packet_queue.empty()) {
		packet_timestamped = packet_queue.front();
		packet_queue.pop();
		packet = packet_timestamped.packet;
		qdelay_current = chrono::duration_cast<chrono::milliseconds>(
		    chrono::high_resolution_clock::now() - packet_timestamped.arrival_time);
	}

	return packet;
}

bool Pi2QueueModule::drop_early(uint8_t ecn) {
	if (packet_queue.size() <= 2) {
		return false;
	}

	double probability = drop_prob;
	// Use pseudo_p if packet supports scalable traffic, indicated by ECT(1);
	// drop_prob otherwise
	if (ecn == ECT1) {
		probability = pseudo_p;
	}

	double u = (*distribution)(generator);
	if (u <= probability) {
		return true;
	} else {
		return false;
	}
}

void Pi2QueueModule::calculate_drop_prob(const boost::system::error_code &error) {
	if (error == boost::asio::error::operation_aborted) {
		return;
	}
	pseudo_p =
	    pseudo_p +
	    alpha *
	        (static_cast<double>((qdelay_current - qdelay_ref).count()) / 1000) +
	    beta * (static_cast<double>((qdelay_current - qdelay_old).count()) / 1000);

	// Bound drop probability [0, 1]
	if (pseudo_p < 0) {
		pseudo_p = 0.0;
	}
	if (pseudo_p > 1) {
		pseudo_p = 1.0;
	}

	drop_prob = pseudo_p * pseudo_p;

	if (drop_prob > MAX_PROB) {
		drop_prob = MAX_PROB;
	}

	qdelay_old = qdelay_current;

	statistic_pseudo_p.set(pseudo_p);
	statistic_drop_prob.set(drop_prob);

	timer_probability_update.expires_at(timer_probability_update.expires_at() +
	                                    t_update);
	timer_probability_update.async_wait(
	    boost::bind(&Pi2QueueModule::calculate_drop_prob,
	                this,
	                boost::asio::placeholders::error));
}

void Pi2QueueModule::statistics(const boost::system::error_code &error) {
	if (error == boost::asio::error::operation_aborted) {
		return;
	}

	statistic_queue_length.set(packet_queue.size());

	statistic_queue_delay.set(qdelay_current.count());
	statistic_packets.set(packets);
	statistic_packets_dropped.set(packets_dropped);
	statistic_packets_marked.set(packets_marked);

	timer_statistics.expires_at(timer_statistics.expiry() +
	                            chrono::milliseconds(1));
	timer_statistics.async_wait(boost::bind(
	    &Pi2QueueModule::statistics, this, boost::asio::placeholders::error));
}
