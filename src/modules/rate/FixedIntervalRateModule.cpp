#include "FixedIntervalRateModule.hpp"

#include <boost/bind.hpp>

using namespace std;

FixedIntervalRateModule::FixedIntervalRateModule(boost::asio::io_service &io_service, chrono::high_resolution_clock::duration interval, size_t buffer_size) : timer_lr(io_service), timer_rl(io_service), interval(interval), buffer_size(buffer_size) {
	chrono::high_resolution_clock::time_point now = chrono::high_resolution_clock::now();

	timer_lr.expires_at(now);
	timer_lr.async_wait(boost::bind(&FixedIntervalRateModule::processLr, this, boost::asio::placeholders::error));

	timer_rl.expires_at(now);
	timer_rl.async_wait(boost::bind(&FixedIntervalRateModule::processRl, this, boost::asio::placeholders::error));
}

void FixedIntervalRateModule::setInterval(chrono::high_resolution_clock::duration interval) {
	this->interval = interval;
}

void FixedIntervalRateModule::setBufferSize(size_t buffer_size) {
	this->buffer_size = buffer_size;
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