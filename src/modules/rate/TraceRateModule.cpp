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

#include "TraceRateModule.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

#include <boost/bind.hpp>

using namespace std;

TraceRateModule::TraceRateModule(boost::asio::io_service &io_service, const string &traces_path, const string &trace_filename_lr, const string &trace_filename_rl) : timer_lr(io_service), timer_rl(io_service) {
	setName("Trace Rate");
	addPort({"lr_in", "In", PortInfo::Side::left, &input_port_lr});
	addPort({"lr_out", "Out", PortInfo::Side::right, &output_port_lr});
	addPort({"rl_in", "In", PortInfo::Side::right, &input_port_rl});
	addPort({"rl_out", "Out", PortInfo::Side::left, &output_port_rl});
	addParameter({"lr_trace_filename", "🠒", "", &parameter_trace_filename_lr});
	addParameter({"rl_trace_filename", "🠐", "", &parameter_trace_filename_rl});

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

void TraceRateModule::listTraces(const string &path) {
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

void TraceRateModule::loadTrace(vector<uint32_t> &trace, const string &trace_filename) {
	trace.clear();

	ifstream trace_file(trace_filename);
	if(!trace_file.is_open()) {
		throw runtime_error("Cannot open trace file: " + trace_filename + "!");
	}

	string line;
	while(getline(trace_file, line)) {
		try {
			trace.push_back(stoul(line));
		} catch(const invalid_argument &e) {
			trace.clear();
			trace_file.close();
			throw runtime_error("Invalid trace file format!");
		}
	}

	trace_file.close();
}

void TraceRateModule::reset() {
	timer_lr.cancel();
	timer_rl.cancel();

	if(trace_lr.empty() || trace_rl.empty()) {
		return;
	}

	trace_lr_itr = trace_lr.begin();
	trace_rl_itr = trace_rl.begin();

	trace_start = chrono::high_resolution_clock::now();

	timer_lr.expires_at(trace_start + chrono::milliseconds(*(trace_lr_itr++)));
	timer_lr.async_wait(boost::bind(&TraceRateModule::processLr, this, boost::asio::placeholders::error));

	timer_rl.expires_at(trace_start + chrono::milliseconds(*(trace_rl_itr++)));
	timer_rl.async_wait(boost::bind(&TraceRateModule::processRl, this, boost::asio::placeholders::error));
}

void TraceRateModule::processLr(const boost::system::error_code& error) {
	if(error == boost::asio::error::operation_aborted) {
		return;
	}

	auto packet = input_port_lr.request();
	if(packet != nullptr) {
		output_port_lr.send(packet);
	}

	unique_lock<mutex> trace_lr_lock(trace_lr_mutex);

	if(trace_lr_itr == trace_lr.end()) {
		if(trace_lr.back() >= trace_rl.back()) {
			unique_lock<mutex> trace_rl_lock(trace_rl_mutex);

			reset();
		}

		return;
	}

	timer_lr.expires_at(trace_start + chrono::milliseconds(*(trace_lr_itr++)));
	timer_lr.async_wait(boost::bind(&TraceRateModule::processLr, this, boost::asio::placeholders::error));
}

void TraceRateModule::processRl(const boost::system::error_code& error) {
	if(error == boost::asio::error::operation_aborted) {
		return;
	}

	auto packet = input_port_rl.request();
	if(packet != nullptr) {
		output_port_rl.send(packet);
	}

	unique_lock<mutex> trace_rl_lock(trace_rl_mutex);

	if(trace_rl_itr == trace_rl.end()) {
		if(trace_rl.back() > trace_lr.back()) {
			unique_lock<mutex> trace_lr_lock(trace_lr_mutex);

			reset();
		}

		return;
	}

	timer_rl.expires_at(trace_start + chrono::milliseconds(*(trace_rl_itr++)));
	timer_rl.async_wait(boost::bind(&TraceRateModule::processRl, this, boost::asio::placeholders::error));
}

TraceRateModule::~TraceRateModule() {
	timer_lr.cancel();
	timer_rl.cancel();
}
