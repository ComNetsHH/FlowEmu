#ifndef FIXED_DELAY_MODULE_HPP
#define FIXED_DELAY_MODULE_HPP

#include <vector>
#include <cstdint>
#include <queue>
#include <utility>
#include <atomic>
#include <chrono>

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "../Module.hpp"
#include "../../utils/Packet.hpp"

class FixedDelayModule : public ModuleHasLeft<std::shared_ptr<Packet>>, public ModuleHasRight<std::shared_ptr<Packet>> {
	public:
		FixedDelayModule(boost::asio::io_service &io_service, uint64_t delay);

		void setDelay(uint64_t delay);

		void receiveFromLeftModule(std::shared_ptr<Packet> packet) override;
		void receiveFromRightModule(std::shared_ptr<Packet> packet) override;
	private:
		std::atomic<uint64_t> delay;

		boost::asio::high_resolution_timer timer;
		std::queue<std::pair<std::chrono::high_resolution_clock::time_point, std::shared_ptr<Packet>>> packet_queue_lr;
		std::queue<std::pair<std::chrono::high_resolution_clock::time_point, std::shared_ptr<Packet>>> packet_queue_rl;

		void setQueueTimeout();
		void processQueue(const boost::system::error_code& error);
};

#endif
