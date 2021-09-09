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

#include "FixedIntervalRateModule.hpp"

#include <boost/bind.hpp>

using namespace std;

FixedIntervalRateModule::FixedIntervalRateModule(boost::asio::io_service &io_service, chrono::high_resolution_clock::duration interval) : timer(io_service) {
	setName("Fixed Interval Rate");
	addPort({"lr_in", "In", PortInfo::Side::left, &input_port_lr});
	addPort({"lr_out", "Out", PortInfo::Side::right, &output_port_lr});
	addPort({"rl_in", "In", PortInfo::Side::right, &input_port_rl});
	addPort({"rl_out", "Out", PortInfo::Side::left, &output_port_rl});
	addParameter({"interval", "Interval", "ms", &parameter_interval});
	addParameter({"rate", "Rate", "packets/s", &parameter_rate});

	parameter_interval.addChangeHandler([&](double value) {
		double rate = parameter_rate.get();
		double rate_from_interval = 1000 / value;

		if(rate != rate_from_interval) {
			parameter_rate.set(rate_from_interval);
		}
	});
	parameter_rate.addChangeHandler([&](double value) {
		double interval = parameter_interval.get();
		double interval_from_rate = 1000 / value;

		if(interval != interval_from_rate) {
			parameter_interval.set(interval_from_rate);
		}
	});

	parameter_interval.set((double) chrono::nanoseconds(interval).count() / 1000000);

	timer.expires_at(chrono::high_resolution_clock::now());
	timer.async_wait(boost::bind(&FixedIntervalRateModule::process, this, boost::asio::placeholders::error));
}

void FixedIntervalRateModule::process(const boost::system::error_code& error) {
	if(error == boost::asio::error::operation_aborted) {
		return;
	}

	auto packet_lr = input_port_lr.request();
	if(packet_lr != nullptr) {
		output_port_lr.send(packet_lr);
	}

	auto packet_rl = input_port_rl.request();
	if(packet_rl != nullptr) {
		output_port_rl.send(packet_rl);
	}

	timer.expires_at(timer.expiry() + chrono::nanoseconds((uint64_t) (parameter_interval.get() * 1000000)));
	timer.async_wait(boost::bind(&FixedIntervalRateModule::process, this, boost::asio::placeholders::error));
}

FixedIntervalRateModule::~FixedIntervalRateModule() {
	timer.cancel();
}
