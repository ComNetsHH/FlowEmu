#include "FixedDelayModule.hpp"

using namespace std;

FixedDelayModule::FixedDelayModule(boost::asio::io_service &io_service, uint64_t delay) : timer(io_service), delay(delay) {
	timer.expires_from_now(boost::posix_time::milliseconds(0));
	timer.async_wait(boost::bind(&FixedDelayModule::processQueue, this));
}

void FixedDelayModule::processQueue() {
	while(packet_queue.size() >= 1) {
		if((packet_queue.front().first + boost::posix_time::milliseconds(delay)) < boost::posix_time::microsec_clock::universal_time()) {
			passToRightModule(packet_queue.front().second);
			packet_queue.pop();
		} else {
			break;
		}
	}

	timer.expires_at(timer.expires_at() + boost::posix_time::microseconds(1));
	timer.async_wait(boost::bind(&FixedDelayModule::processQueue, this));
}

void FixedDelayModule::receiveFromLeftModule(boost::asio::const_buffer packet) {
	packet_queue.emplace(boost::posix_time::microsec_clock::universal_time(), packet);
}

void FixedDelayModule::receiveFromRightModule(boost::asio::const_buffer packet) {
	passToLeftModule(packet);
}
