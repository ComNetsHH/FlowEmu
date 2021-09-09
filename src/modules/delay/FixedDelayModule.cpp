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

#include "FixedDelayModule.hpp"

#include <boost/bind.hpp>

using namespace std;

FixedDelayModule::FixedDelayModule(boost::asio::io_service &io_service, uint64_t delay) : timer_lr(io_service), timer_rl(io_service) {
	setName("Fixed Delay");
	addPort({"lr_in", "In", PortInfo::Side::left, &input_port_lr});
	addPort({"lr_out", "Out", PortInfo::Side::right, &output_port_lr});
	addPort({"rl_in", "In", PortInfo::Side::right, &input_port_rl});
	addPort({"rl_out", "Out", PortInfo::Side::left, &output_port_rl});
	addParameter({"delay", "Delay", "ms", &parameter_delay});

	input_port_lr.setReceiveHandler(bind(&FixedDelayModule::receiveFromLeftModule, this, placeholders::_1));
	input_port_rl.setReceiveHandler(bind(&FixedDelayModule::receiveFromRightModule, this, placeholders::_1));

	parameter_delay.addChangeHandler(bind(&FixedDelayModule::handleDelayChange, this));

	parameter_delay.set(delay);
}

void FixedDelayModule::handleDelayChange() {
	setQueueTimeoutLr();
	setQueueTimeoutRl();
}

void FixedDelayModule::receiveFromLeftModule(shared_ptr<Packet> packet) {
	if(parameter_delay.get() == 0) {
		output_port_lr.send(packet);
		return;
	}

	bool packet_queue_lr_empty = packet_queue_lr.empty();

	packet_queue_lr.emplace(chrono::high_resolution_clock::now(), packet);

	if(packet_queue_lr_empty) {
		setQueueTimeoutLr();
	}
}

void FixedDelayModule::setQueueTimeoutLr() {
	timer_lr.cancel();
	timer_lr.expires_at(packet_queue_lr.front().first + chrono::milliseconds((uint64_t) parameter_delay.get()));
	timer_lr.async_wait(boost::bind(&FixedDelayModule::processQueueLr, this, boost::asio::placeholders::error));
}

void FixedDelayModule::processQueueLr(const boost::system::error_code& error) {
	if(error == boost::asio::error::operation_aborted) {
		return;
	}

	chrono::high_resolution_clock::time_point chrono_deadline = chrono::high_resolution_clock::now() - chrono::milliseconds((uint64_t) parameter_delay.get());

	while(!packet_queue_lr.empty()) {
		if(packet_queue_lr.front().first <= chrono_deadline) {
			output_port_lr.send(packet_queue_lr.front().second);
			packet_queue_lr.pop();
		} else {
			setQueueTimeoutLr();
			break;
		}
	}
}

void FixedDelayModule::receiveFromRightModule(shared_ptr<Packet> packet) {
	if(parameter_delay.get() == 0) {
		output_port_rl.send(packet);
		return;
	}

	bool packet_queue_rl_empty = packet_queue_rl.empty();

	packet_queue_rl.emplace(chrono::high_resolution_clock::now(), packet);

	if(packet_queue_rl_empty) {
		setQueueTimeoutRl();
	}
}

void FixedDelayModule::setQueueTimeoutRl() {
	timer_rl.cancel();
	timer_rl.expires_at(packet_queue_rl.front().first + chrono::milliseconds((uint64_t) parameter_delay.get()));
	timer_rl.async_wait(boost::bind(&FixedDelayModule::processQueueRl, this, boost::asio::placeholders::error));
}

void FixedDelayModule::processQueueRl(const boost::system::error_code& error) {
	if(error == boost::asio::error::operation_aborted) {
		return;
	}

	chrono::high_resolution_clock::time_point chrono_deadline = chrono::high_resolution_clock::now() - chrono::milliseconds((uint64_t) parameter_delay.get());

	while(!packet_queue_rl.empty()) {
		if(packet_queue_rl.front().first <= chrono_deadline) {
			output_port_rl.send(packet_queue_rl.front().second);
			packet_queue_rl.pop();
		} else {
			setQueueTimeoutRl();
			break;
		}
	}
}

FixedDelayModule::~FixedDelayModule() {
	timer_lr.cancel();
	timer_rl.cancel();
}
