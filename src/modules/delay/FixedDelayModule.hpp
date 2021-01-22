#ifndef FIXED_DELAY_MODULE_HPP
#define FIXED_DELAY_MODULE_HPP

#include <vector>
#include <cstdint>
#include <queue>
#include <utility>
#include <atomic>

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

		boost::asio::deadline_timer timer;
		std::queue<std::pair<boost::posix_time::ptime, std::shared_ptr<Packet>>> packet_queue_lr;
		std::queue<std::pair<boost::posix_time::ptime, std::shared_ptr<Packet>>> packet_queue_rl;

		void processQueue();
};

#endif
