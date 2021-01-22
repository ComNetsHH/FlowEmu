#include "FixedDelayModule.hpp"

#include <algorithm>

using namespace std;

FixedDelayModule::FixedDelayModule(boost::asio::io_service &io_service, uint64_t delay) : timer(io_service), delay(delay) {
}

void FixedDelayModule::setDelay(uint64_t delay) {
	this->delay = delay;
	setQueueTimeout();
}

void FixedDelayModule::setQueueTimeout() {
	auto boost_delay = boost::posix_time::milliseconds(delay.load());

	if(!packet_queue_lr.empty() && !packet_queue_rl.empty()) {
		timer.expires_at(min(packet_queue_lr.front().first + boost_delay, packet_queue_rl.front().first + boost_delay));
	} else if(!packet_queue_lr.empty()) {
		timer.expires_at(packet_queue_lr.front().first + boost_delay);
	} else if(!packet_queue_rl.empty()) {
		timer.expires_at(packet_queue_rl.front().first + boost_delay);
	} else {
		return;
	}

	timer.async_wait(boost::bind(&FixedDelayModule::processQueue, this));
}

void FixedDelayModule::processQueue() {
	auto boost_delay = boost::posix_time::milliseconds(delay.load());

	while(!packet_queue_lr.empty()) {
		if(packet_queue_lr.front().first + boost_delay <= boost::posix_time::microsec_clock::universal_time()) {
			passToRightModule(packet_queue_lr.front().second);
			packet_queue_lr.pop();
		} else {
			break;
		}
	}

	while(!packet_queue_rl.empty()) {
		if(packet_queue_rl.front().first + boost_delay <= boost::posix_time::microsec_clock::universal_time()) {
			passToLeftModule(packet_queue_rl.front().second);
			packet_queue_rl.pop();
		} else {
			break;
		}
	}

	setQueueTimeout();
}

void FixedDelayModule::receiveFromLeftModule(shared_ptr<Packet> packet) {
	if(delay == 0) {
		passToRightModule(packet);
		return;
	}

	bool packet_queue_lr_empty = packet_queue_lr.empty();

	packet_queue_lr.emplace(boost::posix_time::microsec_clock::universal_time(), packet);

	if(packet_queue_lr_empty) {
		setQueueTimeout();
	}
}

void FixedDelayModule::receiveFromRightModule(shared_ptr<Packet> packet) {
	if(delay == 0) {
		passToLeftModule(packet);
		return;
	}

	bool packet_queue_rl_empty = packet_queue_rl.empty();

	packet_queue_rl.emplace(boost::posix_time::microsec_clock::universal_time(), packet);

	if(packet_queue_rl_empty) {
		setQueueTimeout();
	}
}
