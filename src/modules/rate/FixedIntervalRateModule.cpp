#include "FixedIntervalRateModule.hpp"

#include <boost/bind.hpp>

using namespace std;

FixedIntervalRateModule::FixedIntervalRateModule(boost::asio::io_service &io_service, Mqtt &mqtt, chrono::high_resolution_clock::duration interval, size_t buffer_size) : timer_lr(io_service), timer_rl(io_service), mqtt(mqtt) {
	setInterval(chrono::nanoseconds(interval));
	setBufferSize(buffer_size);

	mqtt.subscribe("set/fixed_interval_rate_module/interval", [&](const string &topic, const string &message) {
		uint64_t interval = stoul(message);
		setInterval(chrono::nanoseconds(interval));
	});

	mqtt.subscribe("set/fixed_interval_rate_module/rate", [&](const string &topic, const string &message) {
		uint64_t rate = stoul(message);
		setRate(rate);
	});

	mqtt.subscribe("set/fixed_interval_rate_module/buffer_size", [&](const string &topic, const string &message) {
		size_t buffer_size = stoul(message);
		setBufferSize(buffer_size);
	});

	chrono::high_resolution_clock::time_point now = chrono::high_resolution_clock::now();

	timer_lr.expires_at(now);
	timer_lr.async_wait(boost::bind(&FixedIntervalRateModule::processLr, this, boost::asio::placeholders::error));

	timer_rl.expires_at(now);
	timer_rl.async_wait(boost::bind(&FixedIntervalRateModule::processRl, this, boost::asio::placeholders::error));
}

void FixedIntervalRateModule::setInterval(chrono::high_resolution_clock::duration interval) {
	this->interval = interval;

	mqtt.publish("get/fixed_interval_rate_module/interval", to_string(chrono::nanoseconds(interval).count()), true);
	mqtt.publish("get/fixed_interval_rate_module/rate", to_string(1000000000 / chrono::nanoseconds(interval).count()), true);
}

void FixedIntervalRateModule::setRate(uint64_t rate) {
	this->interval = chrono::nanoseconds(1000000000 / rate);

	mqtt.publish("get/fixed_interval_rate_module/interval", to_string(chrono::nanoseconds(interval).count()), true);
	mqtt.publish("get/fixed_interval_rate_module/rate", to_string(1000000000 / chrono::nanoseconds(interval).count()), true);
}

void FixedIntervalRateModule::setBufferSize(size_t buffer_size) {
	this->buffer_size = buffer_size;

	mqtt.publish("get/fixed_interval_rate_module/buffer_size", to_string(buffer_size), true);
}

void FixedIntervalRateModule::receiveFromLeftModule(shared_ptr<Packet> packet) {
	if(packet_queue_lr.size() < buffer_size) {
		packet_queue_lr.push(packet);
	}
}

void FixedIntervalRateModule::processLr(const boost::system::error_code& error) {
	if(error == boost::asio::error::operation_aborted) {
		return;
	}

	if(!packet_queue_lr.empty()) {
		passToRightModule(packet_queue_lr.front());
		packet_queue_lr.pop();
	}

	timer_lr.expires_at(timer_lr.expiry() + interval.load());
	timer_lr.async_wait(boost::bind(&FixedIntervalRateModule::processLr, this, boost::asio::placeholders::error));
}

void FixedIntervalRateModule::receiveFromRightModule(shared_ptr<Packet> packet) {
	if(packet_queue_rl.size() < buffer_size) {
		packet_queue_rl.push(packet);
	}
}

void FixedIntervalRateModule::processRl(const boost::system::error_code& error) {
	if(error == boost::asio::error::operation_aborted) {
		return;
	}

	if(!packet_queue_rl.empty()) {
		passToLeftModule(packet_queue_rl.front());
		packet_queue_rl.pop();
	}

	timer_rl.expires_at(timer_rl.expiry() + interval.load());
	timer_rl.async_wait(boost::bind(&FixedIntervalRateModule::processRl, this, boost::asio::placeholders::error));
}