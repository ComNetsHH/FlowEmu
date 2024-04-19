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

/*
* Implemented using:
*	Nichols, Kathleen and Van Jacobson (2012) Appendix: CoDel pseudocode. https://queue.acm.org/appendices/codel.html
*	Nichols, Kathleen et al. (2018) Controlled Delay Active Queue Management. RFC 8289.
*/

#include "CodelQueueModule.hpp"

#include <boost/bind.hpp>
#include <cmath>

using namespace std;

CodelQueueModule::CodelQueueModule(boost::asio::io_service &io_service,
                                   size_t buffer_size, size_t target_queue_time)
    : timer_statistics(io_service) {
	setName("CoDel Queue");
	addPort({"in", "In", PortInfo::Side::left, &input_port});
	addPort({"out", "Out", PortInfo::Side::right, &output_port});
	addParameter({"ecn_mode", "ECN Mode", "", &parameter_ecn_mode});
	addParameter({"buffer_size", "Buffer", "packets", &parameter_buffer_size});
	addParameter({"target_queue_time", "Target Queueing Time", "ms",
	              &parameter_target_queue_time});
	addStatistic({"queue_length", "Queue", "packets", &statistic_queue_length});
	addStatistic({"count", "Count", "packets", &statistic_count});
	addStatistic(
	    {"sojourn_time", "Packet Sojourn Time", "ms", &statistic_sojourn_time});
	addStatistic({"packets", "Packets Received", "", &statistic_packets});
	addStatistic(
	    {"packets_dropped", "Packets Droppped", "", &statistic_packets_dropped});
	addStatistic(
	    {"packets_marked", "Packets Marked", "", &statistic_packets_marked});

	input_port.setReceiveHandler(
	    bind(&CodelQueueModule::enqueue, this, placeholders::_1));
	output_port.setRequestHandler(bind(&CodelQueueModule::dequeue, this));

	parameter_buffer_size.set(buffer_size);

	parameter_ecn_mode.addChangeHandler([&](bool value) { ecn_mode = value; });
	parameter_ecn_mode.callChangeHandlers();

	parameter_target_queue_time.addChangeHandler(
	    [&](double value) { target = ms(static_cast<uint64_t>(value)); });
	parameter_target_queue_time.callChangeHandlers();

	packets = 0;
	packets_dropped = 0;
	packets_marked = 0;

	// Start statistics timer
	timer_statistics.expires_from_now(ms(0));
	timer_statistics.async_wait(boost::bind(&CodelQueueModule::statistics, this,
	                                        boost::asio::placeholders::error));
}

void CodelQueueModule::enqueue(shared_ptr<Packet> packet) {
	packets++;
	if (packet_queue_timestamp.size() >= parameter_buffer_size.get()) {
		packets_dropped++;
		return;
	}

	auto now = chrono::high_resolution_clock::now();
	auto packet_timestamp = packet_with_timestamp{packet, now};
	packet_queue_timestamp.emplace(packet_timestamp);

	output_port.notify();

	return;
}

std::shared_ptr<Packet> CodelQueueModule::dequeue() {
	auto now = chrono::high_resolution_clock::now();
	dodequeue_result r = dodequeue(now);
	bool ecn_capable_packet = false;

	if (r.packet == nullptr) {
		return r.packet;
	}

	if (ecn_mode) {
		uint8_t ecn = r.packet->getECN();
		if (ecn == 1 || ecn == 2) {
			ecn_capable_packet = true;
		}
	}

	if (dropping_) {
		if (!r.ok_to_drop) {
			dropping_ = false;
		}
		while (now >= drop_next_ && dropping_) {
			count++;
			if (ecn_capable_packet) {
				r.packet->setECN(CONGESTION_EXPERIENCED);
				drop_next_ = control_law(drop_next_, count);
				packets_marked++;
				goto end;
			}
			r = dodequeue(now);
			packets_dropped++;
			if (!r.ok_to_drop) {
				dropping_ = false;
			} else {
				drop_next_ = control_law(drop_next_, count);
			}
		}
	} else if (r.ok_to_drop) {
		if (ecn_capable_packet) {
			r.packet->setECN(CONGESTION_EXPERIENCED);
			packets_marked++;
		} else {
			r = dodequeue(now);
			packets_dropped++;
		}

		dropping_ = true;

		if (now - drop_next_ < 16 * INTERVAL) {
			count = count > 2 ? count - 2 : 1;
		} else {
			count = 1;
		}
		drop_next_ = control_law(now, count);
	}
end:
	return r.packet;
}

CodelQueueModule::time_point CodelQueueModule::control_law(time_point t,
                                                           uint32_t count) {
	return t + std::chrono::nanoseconds(static_cast<int>(
	               std::chrono::duration_cast<std::chrono::nanoseconds>(INTERVAL)
	                   .count() /
	               sqrt(count)));
}

CodelQueueModule::dodequeue_result CodelQueueModule::dodequeue(time_point now) {
	packet_with_timestamp packet_timestamped;
	shared_ptr<Packet> packet;
	packet_timestamped = packet_with_timestamp{packet, now};

	if (!packet_queue_timestamp.empty()) {
		packet_timestamped = packet_queue_timestamp.front();
		packet_queue_timestamp.pop();
	}

	dodequeue_result r = {packet_timestamped.packet, false};
	if (r.packet == nullptr) {
		// queue is empty - we can't be above TARGET
		first_above_time_ = time_point(0ms);
		return r;
	}

	sojourn_time = now - packet_timestamped.arrival_time;
	if ((sojourn_time < target) || (packet_queue_timestamp.size() <= MAXPACKET)) {
		first_above_time_ = time_point(0ms);
	} else {
		if (first_above_time_ == time_point(0ms)) {
			first_above_time_ = now + INTERVAL;
		} else if (now >= first_above_time_) {
			r.ok_to_drop = true;
		}
	}
	return r;
}

void CodelQueueModule::statistics(const boost::system::error_code &error) {
	if (error == boost::asio::error::operation_aborted) {
		return;
	}

	statistic_queue_length.set(packet_queue_timestamp.size());
	statistic_count.set(count);
	statistic_sojourn_time.set(
	    std::chrono::duration_cast<std::chrono::milliseconds>(sojourn_time)
	        .count());
	statistic_packets.set(packets);
	statistic_packets_dropped.set(packets_dropped);
	statistic_packets_marked.set(packets_marked);

	timer_statistics.expires_at(timer_statistics.expiry() +
	                            chrono::milliseconds(1));
	timer_statistics.async_wait(boost::bind(&CodelQueueModule::statistics, this,
	                                        boost::asio::placeholders::error));
}
