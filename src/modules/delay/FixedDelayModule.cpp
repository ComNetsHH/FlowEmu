#include "FixedDelayModule.hpp"

using namespace std;

FixedDelayModule::FixedDelayModule(boost::asio::io_service &io_service, uint64_t delay) : timer_lr(io_service), timer_rl(io_service), delay(delay) {
}

void FixedDelayModule::setDelay(uint64_t delay) {
	this->delay = delay;

	setQueueTimeoutLr();
	setQueueTimeoutRl();
}

void FixedDelayModule::receiveFromLeftModule(shared_ptr<Packet> packet) {
	if(delay == 0) {
		passToRightModule(packet);
		return;
	}

	bool packet_queue_lr_empty = packet_queue_lr.empty();

	packet_queue_lr.emplace(chrono::high_resolution_clock::now(), packet);

	if(packet_queue_lr_empty) {
		setQueueTimeoutLr();
	}
}

void FixedDelayModule::setQueueTimeoutLr() {
	timer_lr.cancel();
	timer_lr.expires_at(packet_queue_lr.front().first + chrono::milliseconds(delay.load()));
	timer_lr.async_wait(boost::bind(&FixedDelayModule::processQueueLr, this, boost::asio::placeholders::error));
}

void FixedDelayModule::processQueueLr(const boost::system::error_code& error) {
	if(error == boost::asio::error::operation_aborted) {
		return;
	}

	chrono::high_resolution_clock::time_point chrono_deadline = chrono::high_resolution_clock::now() - chrono::milliseconds(delay.load());

	while(!packet_queue_lr.empty()) {
		if(packet_queue_lr.front().first <= chrono_deadline) {
			passToRightModule(packet_queue_lr.front().second);
			packet_queue_lr.pop();
		} else {
			setQueueTimeoutLr();
			break;
		}
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
		setQueueTimeoutRl();
	}
}

void FixedDelayModule::setQueueTimeoutRl() {
	timer_rl.cancel();
	timer_rl.expires_at(packet_queue_rl.front().first + chrono::milliseconds(delay.load()));
	timer_rl.async_wait(boost::bind(&FixedDelayModule::processQueueRl, this, boost::asio::placeholders::error));
}

void FixedDelayModule::processQueueRl(const boost::system::error_code& error) {
	if(error == boost::asio::error::operation_aborted) {
		return;
	}

	chrono::high_resolution_clock::time_point chrono_deadline = chrono::high_resolution_clock::now() - chrono::milliseconds(delay.load());

	while(!packet_queue_rl.empty()) {
		if(packet_queue_rl.front().first <= chrono_deadline) {
			passToLeftModule(packet_queue_rl.front().second);
			packet_queue_rl.pop();
		} else {
			setQueueTimeoutRl();
			break;
		}
	}
}