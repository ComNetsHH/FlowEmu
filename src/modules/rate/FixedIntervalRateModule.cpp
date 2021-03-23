#include "FixedIntervalRateModule.hpp"

#include <boost/bind.hpp>

using namespace std;

FixedIntervalRateModule::FixedIntervalRateModule(boost::asio::io_service &io_service, Mqtt &mqtt, chrono::high_resolution_clock::duration interval) : timer_lr(io_service), timer_rl(io_service), mqtt(mqtt) {
	setName("Fixed Interval Rate");
	addPort({"lr_in", "In", PortInfo::Side::left, &input_port_lr});
	addPort({"lr_out", "Out", PortInfo::Side::right, &output_port_lr});
	addPort({"rl_in", "In", PortInfo::Side::right, &input_port_rl});
	addPort({"rl_out", "Out", PortInfo::Side::left, &output_port_rl});

	setInterval(chrono::nanoseconds(interval));

	mqtt.subscribe("set/fixed_interval_rate/interval", [&](const string &topic, const string &message) {
		uint64_t interval = stoul(message);
		setInterval(chrono::nanoseconds(interval));
	});

	mqtt.subscribe("set/fixed_interval_rate/rate", [&](const string &topic, const string &message) {
		uint64_t rate = stoul(message);
		setRate(rate);
	});

	chrono::high_resolution_clock::time_point now = chrono::high_resolution_clock::now();

	timer_lr.expires_at(now);
	timer_lr.async_wait(boost::bind(&FixedIntervalRateModule::processLr, this, boost::asio::placeholders::error));

	timer_rl.expires_at(now);
	timer_rl.async_wait(boost::bind(&FixedIntervalRateModule::processRl, this, boost::asio::placeholders::error));
}

void FixedIntervalRateModule::setInterval(chrono::high_resolution_clock::duration interval) {
	this->interval = interval;

	mqtt.publish("get/fixed_interval_rate/interval", to_string(chrono::nanoseconds(interval).count()), true);
	mqtt.publish("get/fixed_interval_rate/rate", to_string(1000000000 / chrono::nanoseconds(interval).count()), true);
}

void FixedIntervalRateModule::setRate(uint64_t rate) {
	this->interval = chrono::nanoseconds(1000000000 / rate);

	mqtt.publish("get/fixed_interval_rate/interval", to_string(chrono::nanoseconds(interval).count()), true);
	mqtt.publish("get/fixed_interval_rate/rate", to_string(1000000000 / chrono::nanoseconds(interval).count()), true);
}

void FixedIntervalRateModule::processLr(const boost::system::error_code& error) {
	if(error == boost::asio::error::operation_aborted) {
		return;
	}

	auto packet = input_port_lr.request();
	if(packet != nullptr) {
		output_port_lr.send(packet);
	}

	timer_lr.expires_at(timer_lr.expiry() + interval.load());
	timer_lr.async_wait(boost::bind(&FixedIntervalRateModule::processLr, this, boost::asio::placeholders::error));
}

void FixedIntervalRateModule::processRl(const boost::system::error_code& error) {
	if(error == boost::asio::error::operation_aborted) {
		return;
	}

	auto packet = input_port_rl.request();
	if(packet != nullptr) {
		output_port_rl.send(packet);
	}

	timer_rl.expires_at(timer_rl.expiry() + interval.load());
	timer_rl.async_wait(boost::bind(&FixedIntervalRateModule::processRl, this, boost::asio::placeholders::error));
}

FixedIntervalRateModule::~FixedIntervalRateModule() {
	timer_lr.cancel();
	timer_rl.cancel();
}
