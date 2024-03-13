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


#include "PieQueueModule.hpp"

#include <boost/bind.hpp>

using namespace std;

PieQueueModule::PieQueueModule(boost::asio::io_service &io_service,
                               size_t buffer_size, uint64_t qdelay_ref_input,
                               uint64_t max_burst_input, uint32_t seed)
    : timer_probability_update(io_service), timer_statistics(io_service) {
	setName("PIE Queue");
	addPort({"in", "In", PortInfo::Side::left, &input_port});
	addPort({"out", "Out", PortInfo::Side::right, &output_port});
	addParameter({"ecn_mode", "ECN Mode", "", &parameter_ecn_mode});
	addParameter({"buffer_size", "Buffer", "packets", &parameter_buffer_size});
	addParameter({"qdelay_ref", "QDelay", "ms", &parameter_qdelay_ref});
	addParameter({"max_burst", "Max Burst", "ms", &parameter_max_burst});
	addParameter({"seed", "Seed", "", &parameter_seed});
	addStatistic({"queue_length", "Queue", "packets", &statistic_queue_length});
	addStatistic({"drop_prob", "Drop Prob", "", &statistic_drop_prob});
	addStatistic(
	    {"qdelay_current", "Current QDelay", "ms", &statistic_qdelay_current});
	addStatistic(
	    {"burst_allowance", "Burst allowance", "ms", &statistic_burst_allowance});
	addStatistic({"packets", "Packets Received", "", &statistic_packets});
	addStatistic(
	    {"packets_dropped", "Packets Droppped", "", &statistic_packets_dropped});
	addStatistic(
	    {"packets_marked", "Packets Marked", "", &statistic_packets_marked});

	input_port.setReceiveHandler(
	    bind(&PieQueueModule::enqueue, this, placeholders::_1));
	output_port.setRequestHandler(bind(&PieQueueModule::dequeue, this));

	parameter_seed.addChangeHandler([&](double value) {
		generator.seed(value);
	});
	parameter_seed.set(seed);
	distribution.reset(new uniform_real_distribution(0.0, 1.0));

	parameter_ecn_mode.addChangeHandler([&](bool value) { ecn_mode = value; });
	parameter_ecn_mode.callChangeHandlers();

	parameter_buffer_size.set(buffer_size);

	parameter_qdelay_ref.addChangeHandler(
	    [&](uint64_t value) { qdelay_ref = ms(value); });
	parameter_qdelay_ref.callChangeHandlers();

	parameter_max_burst.addChangeHandler(
	    [&](uint64_t value) { max_burst = ms(value); });
	parameter_max_burst.callChangeHandlers();

	qdelay_current = ms(0);
	burst_allowance = ms(0);
	qdelay_old = ms(0);
	packets = 0;
	packets_dropped = 0;
	packets_marked = 0;

	// Start timer to update probability periodically
	timer_probability_update.expires_from_now(0ms);
	timer_probability_update.async_wait(
	    boost::bind(&PieQueueModule::calculate_drop_prob, this,
	                boost::asio::placeholders::error));

	// Start statistics timer
	timer_statistics.expires_from_now(0ms);
	timer_statistics.async_wait(boost::bind(&PieQueueModule::statistics, this,
	                                        boost::asio::placeholders::error));
}

void PieQueueModule::enqueue(shared_ptr<Packet> packet) {
	bool ecn_capable_packet = false;
	if (ecn_mode) {
		uint8_t ecn = packet->getECN();
		if (ecn == 1 || ecn == 2) {
			ecn_capable_packet = true;
		}
	}
	packets++;

	if ((drop_prob == 0) && (qdelay_current < (qdelay_ref / 2)) &&
	    (qdelay_old < (qdelay_ref / 2))) {
		burst_allowance = max_burst;
	}
	if ((burst_allowance == ms(0)) && (drop_early() == true)) {
		if (ecn_capable_packet && (drop_prob <= mark_ecnth)) {
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

std::shared_ptr<Packet> PieQueueModule::dequeue() {
	packet_with_timestamp packet_timestamped;
	shared_ptr<Packet> packet;

	if (!packet_queue.empty()) {
		packet_timestamped = packet_queue.front();
		packet_queue.pop();
		packet = packet_timestamped.packet;
		qdelay_current = chrono::duration_cast<chrono::milliseconds>(
		    chrono::high_resolution_clock::now() - packet_timestamped.arrival_time);
	} else {
		qdelay_current = ms(0);
	}

	return packet;
}

bool PieQueueModule::drop_early() {
	// Dont drop if queue qdel half of reference value and drop_prob is not too
	// high dont drop if queue holds 2 or less packets
	if (((qdelay_old < (qdelay_ref / 2)) && (drop_prob < 0.2)) ||
	    (packet_queue.size() <= 2)) {
		return false;
	}

	double u = (*distribution)(generator);
	if (u <= drop_prob) {
		return true;
	} else {
		return false;
	}
}

void PieQueueModule::calculate_drop_prob(
    const boost::system::error_code &error) {
	if (error == boost::asio::error::operation_aborted) {
		return;
	}
	double p =
	    alpha *
	        (static_cast<double>((qdelay_current - qdelay_ref).count()) / 1000) +
	    beta *
	        (static_cast<double>((qdelay_current - qdelay_old).count()) / 1000);

	// Automatically adjust de-/increase of drop probability to avoid large
	// relative changes
	if (drop_prob < 0.000001) {
		p /= 2048;
	} else if (drop_prob < 0.00001) {
		p /= 512;
	} else if (drop_prob < 0.0001) {
		p /= 128;
	} else if (drop_prob < 0.001) {
		p /= 32;
	} else if (drop_prob < 0.01) {
		p /= 8;
	} else if (drop_prob < 0.1) {
		p /= 2;
	} else {
		p = p;
	}

	drop_prob += p;

	// Decay drop probability if no congestion
	if (qdelay_current == ms(0) && qdelay_old == ms(0)) {
		drop_prob *= 0.98;
	}

	// Bound drop probability [0, 1]
	if (drop_prob < 0) {
		drop_prob = 0.0;
	}
	if (drop_prob > 1) {
		drop_prob = 1.0;
	}

	qdelay_old = qdelay_current;

	if (burst_allowance <= t_update) {
		burst_allowance = ms(0);
	} else {
		burst_allowance = burst_allowance - t_update;
	}

	timer_probability_update.expires_at(timer_probability_update.expires_at() +
	                                    t_update);
	timer_probability_update.async_wait(
	    boost::bind(&PieQueueModule::calculate_drop_prob, this,
	                boost::asio::placeholders::error));
}

void PieQueueModule::statistics(const boost::system::error_code &error) {
	if (error == boost::asio::error::operation_aborted) {
		return;
	}

	statistic_queue_length.set(packet_queue.size());
	statistic_drop_prob.set(drop_prob);
	statistic_qdelay_current.set(qdelay_current.count());
	statistic_burst_allowance.set(burst_allowance.count());

	statistic_packets.set(packets);
	statistic_packets_dropped.set(packets_dropped);
	statistic_packets_marked.set(packets_marked);

	timer_statistics.expires_at(timer_statistics.expiry() + 1ms);
	timer_statistics.async_wait(boost::bind(&PieQueueModule::statistics, this,
	                                        boost::asio::placeholders::error));
}
