#include "TraceRateModule.hpp"

#include <fstream>

#include <boost/bind.hpp>

using namespace std;

TraceRateModule::TraceRateModule(boost::asio::io_service &io_service, Mqtt &mqtt, const string &downlink_trace_filename, const string &uplink_trace_filename) : timer_lr(io_service), timer_rl(io_service), mqtt(mqtt) {
	setName("Trace Rate");
	addPort({"lr_in", "In", PortInfo::Side::left, &input_port_lr});
	addPort({"lr_out", "Out", PortInfo::Side::right, &output_port_lr});
	addPort({"rl_in", "In", PortInfo::Side::right, &input_port_rl});
	addPort({"rl_out", "Out", PortInfo::Side::left, &output_port_rl});

	loadTrace(trace_lr, downlink_trace_filename);
	loadTrace(trace_rl, uplink_trace_filename);

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

void TraceRateModule::reset() {
	timer_lr.cancel();
	timer_rl.cancel();

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

	if(trace_lr_itr == trace_lr.end()) {
		if(trace_lr.back() >= trace_rl.back()) {
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

	if(trace_rl_itr == trace_rl.end()) {
		if(trace_rl.back() > trace_lr.back()) {
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
