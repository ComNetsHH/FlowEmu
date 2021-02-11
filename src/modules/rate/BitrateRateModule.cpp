#include "BitrateRateModule.hpp"

#include <boost/bind.hpp>

using namespace std;

BitrateRateModule::BitrateRateModule(boost::asio::io_service &io_service, Mqtt &mqtt, uint64_t bitrate, size_t buffer_size) : timer_lr(io_service), timer_rl(io_service), mqtt(mqtt) {
	setBitrate(bitrate);
	setBufferSize(buffer_size);

	mqtt.subscribe("set/bitrate_rate/buffer_size", [&](const string &topic, const string &message) {
		size_t buffer_size = stoul(message);
		setBufferSize(buffer_size);
	});

	mqtt.subscribe("set/bitrate_rate/bitrate", [&](const string &topic, const string &message) {
		uint64_t bitrate = stoul(message);
		setBitrate(bitrate);
	});
}

void BitrateRateModule::setBitrate(uint64_t bitrate) {
	this->bitrate = bitrate;

	chrono::high_resolution_clock::time_point now = chrono::high_resolution_clock::now();

	setQueueTimeoutLr(now);
	setQueueTimeoutRl(now);

	mqtt.publish("get/bitrate_rate/bitrate", to_string(bitrate), true);
}

void BitrateRateModule::setBufferSize(size_t buffer_size) {
	this->buffer_size = buffer_size;

	mqtt.publish("get/bitrate_rate/buffer_size", to_string(buffer_size), true);
}

void BitrateRateModule::receiveFromLeftModule(shared_ptr<Packet> packet) {
	if(packet_queue_lr.size() >= buffer_size) {
		return;
	}

	bool packet_queue_lr_empty = packet_queue_lr.empty();

	packet_queue_lr.emplace(packet);

	if(packet_queue_lr_empty) {
		setQueueTimeoutLr(chrono::high_resolution_clock::now());
	}
}

void BitrateRateModule::setQueueTimeoutLr(chrono::high_resolution_clock::time_point now) {
	timer_lr.cancel();

	if(packet_queue_lr.empty()) {
		return;
	}

	uint64_t bitrate_local = bitrate.load();
	if(bitrate_local == 0) {
		return;
	}

	timer_lr.expires_at(now + chrono::nanoseconds((uint64_t) 1000000000 * packet_queue_lr.front()->getBytes().size()*8 / bitrate_local));
	timer_lr.async_wait(boost::bind(&BitrateRateModule::processQueueLr, this, boost::asio::placeholders::error));
}

void BitrateRateModule::processQueueLr(const boost::system::error_code& error) {
	if(error == boost::asio::error::operation_aborted) {
		return;
	}

	passToRightModule(packet_queue_lr.front());
	packet_queue_lr.pop();

	if(!packet_queue_lr.empty()) {
		setQueueTimeoutLr(timer_lr.expiry());
	}
}

void BitrateRateModule::receiveFromRightModule(shared_ptr<Packet> packet) {
	if(packet_queue_rl.size() >= buffer_size) {
		return;
	}

	bool packet_queue_rl_empty = packet_queue_rl.empty();

	packet_queue_rl.emplace(packet);

	if(packet_queue_rl_empty) {
		setQueueTimeoutRl(chrono::high_resolution_clock::now());
	}
}

void BitrateRateModule::setQueueTimeoutRl(chrono::high_resolution_clock::time_point now) {
	timer_rl.cancel();

	if(packet_queue_rl.empty()) {
		return;
	}

	uint64_t bitrate_local = bitrate.load();
	if(bitrate_local == 0) {
		return;
	}

	timer_rl.expires_at(now + chrono::nanoseconds((uint64_t) 1000000000 * packet_queue_rl.front()->getBytes().size()*8 / bitrate_local));
	timer_rl.async_wait(boost::bind(&BitrateRateModule::processQueueRl, this, boost::asio::placeholders::error));
}

void BitrateRateModule::processQueueRl(const boost::system::error_code& error) {
	if(error == boost::asio::error::operation_aborted) {
		return;
	}

	passToLeftModule(packet_queue_rl.front());
	packet_queue_rl.pop();

	if(!packet_queue_rl.empty()) {
		setQueueTimeoutRl(timer_rl.expiry());
	}
}
