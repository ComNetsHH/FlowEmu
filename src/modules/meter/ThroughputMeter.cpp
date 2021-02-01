#include "ThroughputMeter.hpp"

//#include <iostream>

#include <boost/bind.hpp>

using namespace std;

ThroughputMeter::ThroughputMeter(boost::asio::io_service &io_service, Mqtt &mqtt) : timer(io_service), mqtt(mqtt) {
	timer.expires_from_now(chrono::milliseconds(0));
	timer.async_wait(boost::bind(&ThroughputMeter::process, this, boost::asio::placeholders::error));
}

void ThroughputMeter::receiveFromLeftModule(shared_ptr<Packet> packet) {
	auto packet_size = packet->getBytes().size();

	bytes_sum += packet_size;
	bytes.emplace(chrono::high_resolution_clock::now(), packet_size);

	passToRightModule(packet);
}

void ThroughputMeter::process(const boost::system::error_code& error) {
	if(error == boost::asio::error::operation_aborted) {
		return;
	}

	chrono::high_resolution_clock::time_point chrono_deadline = chrono::high_resolution_clock::now() - chrono::seconds(1);

	while(!bytes.empty()) {
		const auto& entry = bytes.front();
		if(entry.first <= chrono_deadline) {
			bytes_sum -= entry.second;
			bytes.pop();
		} else {
			break;
		}
	}

	//cout << bytes_sum << " bytes/s (" << (double) bytes_sum * 8 / 1000 / 1000 << " Mbit/s)" << " - " << bytes.size() << " packets/s" << endl;
	mqtt.publish("get/throughput_meter/left_to_right/byte_per_second", to_string(bytes_sum * 8), true);
	mqtt.publish("get/throughput_meter/left_to_right/packets_per_second", to_string(bytes.size()), true);

	timer.expires_at(timer.expiry() + chrono::milliseconds(100));
	timer.async_wait(boost::bind(&ThroughputMeter::process, this, boost::asio::placeholders::error));
}

void ThroughputMeter::receiveFromRightModule(shared_ptr<Packet> packet) {
	passToLeftModule(packet);
}