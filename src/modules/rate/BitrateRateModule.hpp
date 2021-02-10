#ifndef BITRATE_RATE_MODULE_HPP
#define BITRATE_RATE_MODULE_HPP

#include <cstdint>
#include <queue>
#include <utility>
#include <atomic>
#include <chrono>

#include <boost/asio.hpp>

#include "../Module.hpp"
#include "../../utils/Mqtt.hpp"
#include "../../utils/Packet.hpp"

class BitrateRateModule : public ModuleHasLeft<std::shared_ptr<Packet>>, public ModuleHasRight<std::shared_ptr<Packet>> {
	public:
		BitrateRateModule(boost::asio::io_service &io_service, Mqtt &mqtt, uint64_t bitrate, size_t buffer_size);

		void setBitrate(uint64_t bitrate);
		void setBufferSize(size_t buffer_size);

		void receiveFromLeftModule(std::shared_ptr<Packet> packet) override;
		void receiveFromRightModule(std::shared_ptr<Packet> packet) override;
	private:
		Mqtt &mqtt;

		std::atomic<uint64_t> bitrate;
		std::atomic<size_t> buffer_size;

		boost::asio::high_resolution_timer timer_lr;
		std::queue<std::shared_ptr<Packet>> packet_queue_lr;
		void setQueueTimeoutLr(std::chrono::high_resolution_clock::time_point now);
		void processQueueLr(const boost::system::error_code& error);

		boost::asio::high_resolution_timer timer_rl;
		std::queue<std::shared_ptr<Packet>> packet_queue_rl;
		void setQueueTimeoutRl(std::chrono::high_resolution_clock::time_point now);
		void processQueueRl(const boost::system::error_code& error);
};

#endif
