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

#include "ThroughputMeter.hpp"

#include <boost/bind.hpp>

using namespace std;

ThroughputMeter::ThroughputMeter(boost::asio::io_service &io_service) : timer(io_service) {
	setName("Throughput Meter");
	addPort({"in", "In", PortInfo::Side::left, &input_port});
	addPort({"out", "Out", PortInfo::Side::right, &output_port});
	addParameter({"interval", "Interval", "ms", &parameter_interval});
	addParameter({"window_size", "Window", "ms", &parameter_window_size});
	addStatistic({"bits_per_second", "Bits", "bit/s", &statistic_bits_per_second});
	addStatistic({"bytes_per_second", "Bytes", "B/s", &statistic_bytes_per_second});
	addStatistic({"packets_per_second", "Packets", "packets/s", &statistic_packets_per_second});

	input_port.setReceiveHandler(bind(&ThroughputMeter::receive, this, placeholders::_1));

	timer.expires_from_now(chrono::milliseconds(0));
	timer.async_wait(boost::bind(&ThroughputMeter::process, this, boost::asio::placeholders::error));
}

void ThroughputMeter::receive(shared_ptr<Packet> packet) {
	auto packet_size = packet->getBytes().size();

	bytes_sum += packet_size;
	bytes.emplace(chrono::high_resolution_clock::now(), packet_size);

	output_port.send(packet);
}

void ThroughputMeter::process(const boost::system::error_code& error) {
	if(error == boost::asio::error::operation_aborted) {
		return;
	}

	chrono::high_resolution_clock::time_point chrono_deadline = chrono::high_resolution_clock::now() - chrono::milliseconds((uint64_t) parameter_window_size.get());
	while(!bytes.empty()) {
		const auto& entry = bytes.front();
		if(entry.first <= chrono_deadline) {
			bytes_sum -= entry.second;
			bytes.pop();
		} else {
			break;
		}
	}

	double window_factor = 1000.0 / parameter_window_size.get();
	uint64_t bytes_per_second = bytes_sum * window_factor;
	statistic_bits_per_second.set(bytes_per_second * 8);
	statistic_bytes_per_second.set(bytes_per_second);
	statistic_packets_per_second.set(bytes.size() * window_factor);

	timer.expires_at(timer.expiry() + chrono::milliseconds((uint64_t) parameter_interval.get()));
	timer.async_wait(boost::bind(&ThroughputMeter::process, this, boost::asio::placeholders::error));
}

ThroughputMeter::~ThroughputMeter() {
	timer.cancel();
}
