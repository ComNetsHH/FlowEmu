#include "FixedDelayModule.hpp"

using namespace std;

FixedDelayModule::FixedDelayModule(boost::asio::io_service &io_service, uint64_t delay) : timer(io_service), delay(delay) {
	timer.expires_from_now(boost::posix_time::milliseconds(0));
	timer.async_wait(boost::bind(&FixedDelayModule::processQueue, this));
}

void FixedDelayModule::processQueue() {
	while(packet_queue_lr.size() >= 1) {
		if((packet_queue_lr.front().first + boost::posix_time::milliseconds(delay)) < boost::posix_time::microsec_clock::universal_time()) {
			passToRightModule(packet_queue_lr.front().second);
			packet_queue_lr.pop();
		} else {
			break;
		}
	}

	while(packet_queue_rl.size() >= 1) {
		if((packet_queue_rl.front().first + boost::posix_time::milliseconds(delay)) < boost::posix_time::microsec_clock::universal_time()) {
			passToLeftModule(packet_queue_rl.front().second);
			packet_queue_rl.pop();
		} else {
			break;
		}
	}

	timer.expires_at(timer.expires_at() + boost::posix_time::microseconds(1));
	timer.async_wait(boost::bind(&FixedDelayModule::processQueue, this));
}

void FixedDelayModule::receiveFromLeftModule(shared_ptr<Packet> packet) {
	packet_queue_lr.emplace(boost::posix_time::microsec_clock::universal_time(), packet);
}

void FixedDelayModule::receiveFromRightModule(shared_ptr<Packet> packet) {
	packet_queue_rl.emplace(boost::posix_time::microsec_clock::universal_time(), packet);
}
