#include "ThroughputMeter.hpp"

//#include <iostream>

#include <boost/bind.hpp>

using namespace std;

ThroughputMeter::ThroughputMeter(boost::asio::io_service &io_service, Mqtt &mqtt, const string &module_id) : timer(io_service), mqtt(mqtt), module_id(module_id) {
	setName("Throughput Meter");
	addPort({"in", "In", PortInfo::Side::left, &input_port});
	addPort({"out", "Out", PortInfo::Side::right, &output_port});

	input_port.setReceiveHandler(bind(&ThroughputMeter::receive, this, placeholders::_1));

	timer.expires_from_now(chrono::milliseconds(0));
	timer.async_wait(boost::bind(&ThroughputMeter::process, this, boost::asio::placeholders::error));
}

void ThroughputMeter::receive(shared_ptr<Packet> packet) {
	auto packet_size = packet->getBytes().size();

	bytes_sum += packet_size;
	bytes.emplace(chrono::high_resolution_clock::now(), packet_size);

	output_port.send(packet);
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
	mqtt.publish("get/" + module_id + "/bytes_per_second", to_string(bytes_sum), true);
	mqtt.publish("get/" + module_id + "/packets_per_second", to_string(bytes.size()), true);

	timer.expires_at(timer.expiry() + chrono::milliseconds(100));
	timer.async_wait(boost::bind(&ThroughputMeter::process, this, boost::asio::placeholders::error));
}

ThroughputMeter::~ThroughputMeter() {
	timer.cancel();
}
