#include "DelayMeter.hpp"

#include <iostream>
#include <limits>

#include <boost/bind.hpp>

using namespace std;

DelayMeter::DelayMeter(boost::asio::io_service &io_service) : timer(io_service) {
	timer.expires_from_now(chrono::milliseconds(0));
	timer.async_wait(boost::bind(&DelayMeter::process, this, boost::asio::placeholders::error));
}

void DelayMeter::receiveFromLeftModule(shared_ptr<Packet> packet) {
	creation_time_points.emplace_back(chrono::high_resolution_clock::now(), packet->getCreationTimePoint());

	passToRightModule(packet);
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

	double min_delay = numeric_limits<double>::max();
	double max_delay = numeric_limits<double>::min();
	double mean_delay = 0;
	for(const auto& entry: creation_time_points) {
		double delay = chrono::nanoseconds(entry.first - entry.second).count();

		if(delay < min_delay) {min_delay = delay;}
		if(delay > max_delay) {max_delay = delay;}
		mean_delay += delay;
	}
	mean_delay /= creation_time_points.size();

	cout << "Min: " << min_delay / 1000000 << " ms | Mean: " << mean_delay / 1000000 << " ms | Max: " << max_delay / 1000000 << " ms" << endl;

	timer.expires_at(timer.expiry() + chrono::milliseconds(100));
	timer.async_wait(boost::bind(&DelayMeter::process, this, boost::asio::placeholders::error));
}

void DelayMeter::receiveFromRightModule(shared_ptr<Packet> packet) {
	passToLeftModule(packet);
}