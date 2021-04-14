#include "BitrateRateModule.hpp"

#include <boost/bind.hpp>

using namespace std;

BitrateRateModule::BitrateRateModule(boost::asio::io_service &io_service, Mqtt &mqtt, uint64_t bitrate) : timer(io_service), mqtt(mqtt) {
	setName("Bitrate Rate");
	addPort({"in", "In", PortInfo::Side::left, &input_port});
	addPort({"out", "Out", PortInfo::Side::right, &output_port});

	input_port.setNotifyHandler([&]() {
		if(active) {
			return;
		}

		timer.expires_at(chrono::high_resolution_clock::now());
		timer.async_wait(boost::bind(&BitrateRateModule::process, this, boost::asio::placeholders::error));
	});

	setBitrate(bitrate);

	mqtt.subscribe("set/bitrate_rate/bitrate", [&](const string &topic, const string &message) {
		uint64_t bitrate = stoul(message);
		setBitrate(bitrate);
	});
}

void BitrateRateModule::setBitrate(uint64_t bitrate) {
	this->bitrate = bitrate;

	mqtt.publish("get/bitrate_rate/bitrate", to_string(bitrate), true);
}

void BitrateRateModule::process(const boost::system::error_code& error) {
	if(error == boost::asio::error::operation_aborted) {
		return;
	}

	uint64_t bitrate_local = bitrate.load();
	if(bitrate_local == 0) {
		return;
	}

	auto packet = input_port.request();
	if(packet != nullptr) {
		output_port.send(packet);

		active = true;

		timer.expires_at(timer.expiry() + chrono::nanoseconds((uint64_t) 1000000000 * packet->getBytes().size()*8 / bitrate_local));
		timer.async_wait(boost::bind(&BitrateRateModule::process, this, boost::asio::placeholders::error));
	} else {
		active = false;
	}
}

BitrateRateModule::~BitrateRateModule() {
	timer.cancel();
}
