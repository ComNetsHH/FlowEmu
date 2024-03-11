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

#include "TraceLossModule.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

#include <boost/bind.hpp>

using namespace std;

TraceLossModule::TraceLossModule(boost::asio::io_service &io_service, const string &traces_path, const string &trace_filename_lr, const string &trace_filename_rl) {
	setName("Trace Loss");
	addPort({"lr_in", "In", PortInfo::Side::left, &input_port_lr});
	addPort({"lr_out", "Out", PortInfo::Side::right, &output_port_lr});
	addPort({"rl_in", "In", PortInfo::Side::right, &input_port_rl});
	addPort({"rl_out", "Out", PortInfo::Side::left, &output_port_rl});
	addParameter({"lr_trace_filename", "🠒", "", &parameter_trace_filename_lr});
	addParameter({"rl_trace_filename", "🠐", "", &parameter_trace_filename_rl});

	input_port_lr.setReceiveHandler(bind(&TraceLossModule::receiveFromLeftModule, this, placeholders::_1));
	input_port_rl.setReceiveHandler(bind(&TraceLossModule::receiveFromRightModule, this, placeholders::_1));

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

void TraceLossModule::listTraces(const string &path) {
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

void TraceLossModule::loadTrace(vector<bool> &trace, const string &trace_filename) {
	trace.clear();

	ifstream trace_file(trace_filename);
	if(!trace_file.is_open()) {
		throw runtime_error("Cannot open trace file: " + trace_filename + "!");
	}

	string line;
	while(getline(trace_file, line)) {
		for(const auto& c : line) {
			if(c == '1') {
				trace.push_back(true);
			} else if(c == '0') {
				trace.push_back(false);
			}
		}
	}

	trace_file.close();
}

void TraceLossModule::reset() {
	if(trace_lr.empty() || trace_rl.empty()) {
		return;
	}

	trace_lr_itr = trace_lr.begin();
	trace_rl_itr = trace_rl.begin();
}

void TraceLossModule::receiveFromLeftModule(shared_ptr<Packet> packet) {
	unique_lock<mutex> trace_lr_lock(trace_lr_mutex);

	if(trace_lr_itr == trace_lr.end()) {
		trace_lr_itr = trace_lr.begin();
	}

	if(*(trace_lr_itr++)) {
		output_port_lr.send(packet);
	}
}

void TraceLossModule::receiveFromRightModule(shared_ptr<Packet> packet) {
	unique_lock<mutex> trace_rl_lock(trace_rl_mutex);

	if(trace_rl_itr == trace_rl.end()) {
		trace_rl_itr = trace_rl.begin();
	}

	if(*(trace_rl_itr++)) {
		output_port_rl.send(packet);
	}
}
