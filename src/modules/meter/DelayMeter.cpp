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

#include "DelayMeter.hpp"

#include <limits>

#include <boost/bind.hpp>

using namespace std;

DelayMeter::DelayMeter(boost::asio::io_service &io_service) : timer(io_service) {
	setName("Delay Meter");
	addPort({"in", "In", PortInfo::Side::left, &input_port});
	addPort({"out", "Out", PortInfo::Side::right, &output_port});
	addParameter({"interval", "Interval", "ms", &parameter_interval});
	addParameter({"window_size", "Window", "ms", &parameter_window_size});
	addStatistic({"min", "Min.", "ms", &statistic_min});
	addStatistic({"max", "Max.", "ms", &statistic_max});
	addStatistic({"mean", "Mean", "ms", &statistic_mean});

	input_port.setReceiveHandler(bind(&DelayMeter::receive, this, placeholders::_1));

	timer.expires_from_now(chrono::milliseconds(0));
	timer.async_wait(boost::bind(&DelayMeter::process, this, boost::asio::placeholders::error));
}

void DelayMeter::receive(shared_ptr<Packet> packet) {
	creation_time_points.emplace_back(chrono::high_resolution_clock::now(), packet->getCreationTimePoint());

	output_port.send(packet);
}

void DelayMeter::process(const boost::system::error_code& error) {
	if(error == boost::asio::error::operation_aborted) {
		return;
	}

	chrono::high_resolution_clock::time_point chrono_deadline = chrono::high_resolution_clock::now() - chrono::milliseconds((uint64_t) parameter_window_size.get());
	while(!creation_time_points.empty()) {
		const auto& entry = creation_time_points.front();
		if(entry.first <= chrono_deadline) {
			creation_time_points.pop_front();
		} else {
			break;
		}
	}

	int64_t min_delay = numeric_limits<int64_t>::max();
	int64_t max_delay = numeric_limits<int64_t>::min();
	double mean_delay = 0;

	for(const auto& entry: creation_time_points) {
		int64_t delay = chrono::nanoseconds(entry.first - entry.second).count();

		if(delay < min_delay) {min_delay = delay;}
		if(delay > max_delay) {max_delay = delay;}
		mean_delay += delay;
	}

	if(min_delay == numeric_limits<int64_t>::max()) {
		min_delay = 0;
	}

	if(max_delay == numeric_limits<int64_t>::min()) {
		max_delay = 0;
	}

	auto samples_num = creation_time_points.size();
	if(samples_num != 0) {
		mean_delay /= samples_num;
	}

	statistic_min.set((double) min_delay / 1000000);
	statistic_max.set((double) max_delay / 1000000);
	statistic_mean.set(mean_delay / 1000000);

	timer.expires_at(timer.expiry() + chrono::milliseconds((uint64_t) parameter_interval.get()));
	timer.async_wait(boost::bind(&DelayMeter::process, this, boost::asio::placeholders::error));
}

DelayMeter::~DelayMeter() {
	timer.cancel();
}
