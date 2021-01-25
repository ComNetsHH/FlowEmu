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
	chrono::high_resolution_clock::duration chrono_delay = chrono::milliseconds(delay.load());

	timer.cancel();

	if(!packet_queue_lr.empty() && !packet_queue_rl.empty()) {
		timer.expires_at(min(packet_queue_lr.front().first + chrono_delay, packet_queue_rl.front().first + chrono_delay));
	} else if(!packet_queue_lr.empty()) {
		timer.expires_at(packet_queue_lr.front().first + chrono_delay);
	} else if(!packet_queue_rl.empty()) {
		timer.expires_at(packet_queue_rl.front().first + chrono_delay);
	} else {
		return;
	}

	timer.async_wait(boost::bind(&FixedDelayModule::processQueue, this, boost::asio::placeholders::error));
}

void FixedDelayModule::processQueue(const boost::system::error_code& error) {
	if(error == boost::asio::error::operation_aborted) {
		return;
	}

	chrono::high_resolution_clock::duration chrono_delay = chrono::milliseconds(delay.load());
	chrono::high_resolution_clock::time_point chrono_time = chrono::high_resolution_clock::now();

	while(!packet_queue_lr.empty()) {
		auto packet_deadline = chrono::duration_cast<chrono::nanoseconds>((packet_queue_lr.front().first + chrono_delay) - chrono_time).count();
		if(packet_deadline <= 0) {
			passToRightModule(packet_queue_lr.front().second);
			packet_queue_lr.pop();
		} else {
			break;
		}
	}

	while(!packet_queue_rl.empty()) {
		auto packet_deadline = chrono::duration_cast<chrono::nanoseconds>((packet_queue_rl.front().first + chrono_delay) - chrono_time).count();
		if(packet_deadline <= 0) {
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

	packet_queue_lr.emplace(chrono::high_resolution_clock::now(), packet);

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

	packet_queue_rl.emplace(chrono::high_resolution_clock::now(), packet);

	if(packet_queue_rl_empty) {
		setQueueTimeout();
	}
}
