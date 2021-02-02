#ifndef SLOTTED_RATE_MODULE_HPP
#define SLOTTED_RATE_MODULE_HPP

#include <vector>
#include <cstdint>
#include <queue>
#include <memory>

#include <boost/asio.hpp>

#include "../Module.hpp"
#include "../../utils/Mqtt.hpp"
#include "../../utils/Packet.hpp"

class FixedIntervalRateModule : public ModuleHasLeft<std::shared_ptr<Packet>>, public ModuleHasRight<std::shared_ptr<Packet>> {
	public:
		FixedIntervalRateModule(boost::asio::io_service &io_service, Mqtt &mqtt, std::chrono::high_resolution_clock::duration interval, size_t buffer_size);

		void setInterval(std::chrono::high_resolution_clock::duration interval);
		void setRate(uint64_t rate);
		void setBufferSize(size_t buffer_size);

		void receiveFromLeftModule(std::shared_ptr<Packet> packet) override;
		void receiveFromRightModule(std::shared_ptr<Packet> packet) override;

	private:
		Mqtt &mqtt;

		std::atomic<std::chrono::high_resolution_clock::duration> interval;
		std::atomic<size_t> buffer_size;

		boost::asio::high_resolution_timer timer_lr;
		std::queue<std::shared_ptr<Packet>> packet_queue_lr;
		void processLr(const boost::system::error_code& error);

		boost::asio::high_resolution_timer timer_rl;
		std::queue<std::shared_ptr<Packet>> packet_queue_rl;
		void processRl(const boost::system::error_code& error);
};

#endif
