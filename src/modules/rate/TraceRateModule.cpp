#include "TraceRateModule.hpp"

#include <fstream>

#include <boost/bind.hpp>

using namespace std;

TraceRateModule::TraceRateModule(boost::asio::io_service &io_service, Mqtt &mqtt, string downlink_trace_filename, string uplink_trace_filename, size_t buffer_size) : timer_lr(io_service), timer_rl(io_service), mqtt(mqtt) {
	loadTrace(trace_lr, downlink_trace_filename);
	loadTrace(trace_rl, uplink_trace_filename);
	setBufferSize(buffer_size);

	mqtt.subscribe("set/trace_rate/buffer_size", [&](const string &topic, const string &message) {
		size_t buffer_size = stoul(message);
		setBufferSize(buffer_size);
	});

	mqtt.subscribe("set/trace_rate", [&](const string &topic, const string &message) {
		if(message == "reset") {
			reset();
		}
	});

	reset();
}

void TraceRateModule::loadTrace(vector<uint32_t> &trace, const string &trace_filename) {
	ifstream trace_file(trace_filename);
	if(!trace_file.is_open()) {
		throw runtime_error("Cannot open trace file: " + trace_filename + "!");
	}

	trace.clear();

	string line;
	while(getline(trace_file, line)) {
		trace.push_back(stoul(line));
	}

	trace_file.close();
}

void TraceRateModule::setBufferSize(size_t buffer_size) {
	this->buffer_size = buffer_size;

	mqtt.publish("get/trace_rate/buffer_size", to_string(buffer_size), true);
}

void TraceRateModule::reset() {
	timer_lr.cancel();
	timer_rl.cancel();

	trace_lr_itr = trace_lr.begin();
	trace_rl_itr = trace_rl.begin();

	trace_lr_start = chrono::high_resolution_clock::now();
	timer_lr.expires_at(trace_lr_start + chrono::milliseconds(*(trace_lr_itr++)));
	timer_lr.async_wait(boost::bind(&TraceRateModule::processLr, this, boost::asio::placeholders::error));

	trace_rl_start = chrono::high_resolution_clock::now();
	timer_rl.expires_at(trace_rl_start + chrono::milliseconds(*(trace_rl_itr++)));
	timer_rl.async_wait(boost::bind(&TraceRateModule::processRl, this, boost::asio::placeholders::error));
}

void TraceRateModule::receiveFromLeftModule(shared_ptr<Packet> packet) {
	if(packet_queue_lr.size() < buffer_size) {
		packet_queue_lr.push(packet);
	}
}

void TraceRateModule::processLr(const boost::system::error_code& error) {
	if(error == boost::asio::error::operation_aborted) {
		return;
	}

	if(!packet_queue_lr.empty()) {
		passToRightModule(packet_queue_lr.front());
		packet_queue_lr.pop();
	}

	if(trace_lr_itr == trace_lr.end()) {
		trace_lr_itr = trace_lr.begin();
		trace_lr_start = chrono::high_resolution_clock::now();
	}

	timer_lr.expires_at(trace_lr_start + chrono::milliseconds(*(trace_lr_itr++)));
	timer_lr.async_wait(boost::bind(&TraceRateModule::processLr, this, boost::asio::placeholders::error));
}

void TraceRateModule::receiveFromRightModule(shared_ptr<Packet> packet) {
	if(packet_queue_rl.size() < buffer_size) {
		packet_queue_rl.push(packet);
	}
}

void TraceRateModule::processRl(const boost::system::error_code& error) {
	if(error == boost::asio::error::operation_aborted) {
		return;
	}

	if(!packet_queue_rl.empty()) {
		passToLeftModule(packet_queue_rl.front());
		packet_queue_rl.pop();
	}

	if(trace_rl_itr == trace_rl.end()) {
		trace_rl_itr = trace_rl.begin();
		trace_rl_start = chrono::high_resolution_clock::now();
	}

	timer_rl.expires_at(trace_rl_start + chrono::milliseconds(*(trace_rl_itr++)));
	timer_rl.async_wait(boost::bind(&TraceRateModule::processRl, this, boost::asio::placeholders::error));
}