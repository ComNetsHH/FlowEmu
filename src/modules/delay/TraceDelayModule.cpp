/*
 * FlowEmu - Flow-Based Network Emulator
 * Copyright (c) 2024 Institute of Communication Networks (ComNets),
 *                    Hamburg University of Technology (TUHH),
 *                    https://www.tuhh.de/comnets
 * Copyright (c) 2024 Daniel Stolpmann <daniel.stolpmann@tuhh.de>
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

#include "TraceDelayModule.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

#include <boost/bind.hpp>

using namespace std;

TraceDelayModule::TraceDelayModule(boost::asio::io_service &io_service, const string &traces_path, const string &trace_filename_lr, const string &trace_filename_rl) : timer_lr(io_service), timer_rl(io_service) {
	setName("Trace Delay");
	addPort({"lr_in", "In", PortInfo::Side::left, &input_port_lr});
	addPort({"lr_out", "Out", PortInfo::Side::right, &output_port_lr});
	addPort({"rl_in", "In", PortInfo::Side::right, &input_port_rl});
	addPort({"rl_out", "Out", PortInfo::Side::left, &output_port_rl});
	addParameter({"lr_trace_filename", "🠒", "", &parameter_trace_filename_lr});
	addParameter({"rl_trace_filename", "🠐", "", &parameter_trace_filename_rl});

	input_port_lr.setReceiveHandler(bind(&TraceDelayModule::receiveFromLeftModule, this, placeholders::_1));
	input_port_rl.setReceiveHandler(bind(&TraceDelayModule::receiveFromRightModule, this, placeholders::_1));

	parameter_trace_filename_lr.addChangeHandler([&, traces_path](string trace_filename_lr) {
		unique_lock<mutex> trace_lr_lock(trace_lr_mutex);

		try {
			loadTrace(trace_lr, traces_path + "/" + trace_filename_lr);
		} catch(const runtime_error &e) {
			cerr << e.what() << endl;
		}

		reset();
	});
	parameter_trace_filename_rl.addChangeHandler([&, traces_path](string trace_filename_rl) {
		unique_lock<mutex> trace_rl_lock(trace_rl_mutex);

		try {
			loadTrace(trace_rl, traces_path + "/" + trace_filename_rl);
		} catch(const runtime_error &e) {
			cerr << e.what() << endl;
		}

		reset();
	});

	parameter_trace_filename_lr.set(trace_filename_lr);
	parameter_trace_filename_rl.set(trace_filename_rl);

	listTraces(traces_path);
}

void TraceDelayModule::listTraces(const string &path) {
	if(!filesystem::exists(path)) {
		cerr << "Path " << path << " does not exist!" << endl;
		return;
	}

	std::list<std::string> trace_filenames;
	for(const auto &entry : filesystem::directory_iterator(path)) {
		if(entry.path().filename().extension() == ".md") {
			continue;
		}

		trace_filenames.push_back(entry.path().filename().string());
	}

	trace_filenames.sort();

	parameter_trace_filename_lr.setOptions(trace_filenames);
	parameter_trace_filename_rl.setOptions(trace_filenames);
}

void TraceDelayModule::loadTrace(vector<uint32_t> &trace, const string &trace_filename) {
	trace.clear();

	ifstream trace_file(trace_filename);
	if(!trace_file.is_open()) {
		throw runtime_error("Cannot open trace file: " + trace_filename + "!");
	}

	auto parseValue = [&](string &value) {
		if(value.empty()) {
			return;
		}

		try {
			trace.push_back(stoul(value));
		} catch(const invalid_argument &e) {
			trace.clear();
			trace_file.close();
			throw runtime_error("Invalid trace file format!");
		}

		value.clear();
	};

	string line;
	while(getline(trace_file, line)) {
		string value;
		for(const auto& c : line) {
			if(c != ' ') {
				value.push_back(c);
			} else {
				parseValue(value);
			}
		}

		parseValue(value);
	}

	trace_file.close();
}

void TraceDelayModule::reset() {
	if(trace_lr.empty() || trace_rl.empty()) {
		return;
	}

	trace_lr_itr = trace_lr.begin();
	trace_rl_itr = trace_rl.begin();

	setQueueTimeoutLr();
	setQueueTimeoutRl();
}

void TraceDelayModule::receiveFromLeftModule(shared_ptr<Packet> packet) {
	unique_lock<mutex> trace_lr_lock(trace_lr_mutex);

	if(trace_lr_itr == trace_lr.end()) {
		trace_lr_itr = trace_lr.begin();
	}
	uint32_t delay = *(trace_lr_itr++);

	bool packet_queue_lr_empty = packet_queue_lr.empty();

	packet_queue_lr.emplace(chrono::high_resolution_clock::now() + chrono::nanoseconds((uint64_t) delay * 1000000UL), packet);

	if(packet_queue_lr_empty) {
		setQueueTimeoutLr();
	}
}

void TraceDelayModule::setQueueTimeoutLr() {
	timer_lr.cancel();
	timer_lr.expires_at(packet_queue_lr.front().first);
	timer_lr.async_wait(boost::bind(&TraceDelayModule::processQueueLr, this, boost::asio::placeholders::error));
}

void TraceDelayModule::processQueueLr(const boost::system::error_code& error) {
	if(error == boost::asio::error::operation_aborted) {
		return;
	}

	chrono::high_resolution_clock::time_point chrono_deadline = chrono::high_resolution_clock::now();

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

void TraceDelayModule::receiveFromRightModule(shared_ptr<Packet> packet) {
	unique_lock<mutex> trace_rl_lock(trace_rl_mutex);

	if(trace_rl_itr == trace_rl.end()) {
		trace_rl_itr = trace_rl.begin();
	}
	uint32_t delay = *(trace_rl_itr++);

	bool packet_queue_rl_empty = packet_queue_rl.empty();

	packet_queue_rl.emplace(chrono::high_resolution_clock::now() + chrono::nanoseconds((uint64_t) delay * 1000000UL), packet);

	if(packet_queue_rl_empty) {
		setQueueTimeoutRl();
	}
}

void TraceDelayModule::setQueueTimeoutRl() {
	timer_rl.cancel();
	timer_rl.expires_at(packet_queue_rl.front().first);
	timer_rl.async_wait(boost::bind(&TraceDelayModule::processQueueRl, this, boost::asio::placeholders::error));
}

void TraceDelayModule::processQueueRl(const boost::system::error_code& error) {
	if(error == boost::asio::error::operation_aborted) {
		return;
	}

	chrono::high_resolution_clock::time_point chrono_deadline = chrono::high_resolution_clock::now();

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

TraceDelayModule::~TraceDelayModule() {
	timer_lr.cancel();
	timer_rl.cancel();
}
