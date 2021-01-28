#ifndef SLOTTED_RATE_MODULE_HPP
#define SLOTTED_RATE_MODULE_HPP

#include <vector>
#include <cstdint>
#include <queue>
#include <memory>

#include <boost/asio.hpp>

#include "../Module.hpp"
#include "../../utils/Packet.hpp"

class FixedIntervalRateModule : public ModuleHasLeft<std::shared_ptr<Packet>>, public ModuleHasRight<std::shared_ptr<Packet>> {
	public:
		FixedIntervalRateModule(boost::asio::io_service &io_service);

		void receiveFromLeftModule(std::shared_ptr<Packet> packet) override;
		void receiveFromRightModule(std::shared_ptr<Packet> packet) override;

	private:
		boost::asio::high_resolution_timer timer_lr;
		std::queue<std::shared_ptr<Packet>> packet_queue_lr;
		void processLr(const boost::system::error_code& error);

		boost::asio::high_resolution_timer timer_rl;
		std::queue<std::shared_ptr<Packet>> packet_queue_rl;
		void processRl(const boost::system::error_code& error);
};

#endif
