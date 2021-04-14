#include "DelayMeter.hpp"

//#include <iostream>
#include <limits>

#include <boost/bind.hpp>

using namespace std;

DelayMeter::DelayMeter(boost::asio::io_service &io_service, Mqtt &mqtt) : timer(io_service), mqtt(mqtt) {
	setName("Delay Meter");
	addPort({"in", "In", PortInfo::Side::left, &input_port});
	addPort({"out", "Out", PortInfo::Side::right, &output_port});

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

	chrono::high_resolution_clock::time_point chrono_deadline = chrono::high_resolution_clock::now() - chrono::seconds(1);
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

	//cout << "Min: " << min_delay / 1000000 << " ms | Mean: " << mean_delay / 1000000 << " ms | Max: " << max_delay / 1000000 << " ms" << endl;
	mqtt.publish("get/delay_meter/min", to_string(min_delay), true);
	mqtt.publish("get/delay_meter/max", to_string(max_delay), true);
	mqtt.publish("get/delay_meter/mean", to_string(mean_delay), true);

	timer.expires_at(timer.expiry() + chrono::milliseconds(100));
	timer.async_wait(boost::bind(&DelayMeter::process, this, boost::asio::placeholders::error));
}

DelayMeter::~DelayMeter() {
	timer.cancel();
}
