#ifndef FIXED_DELAY_MODULE_HPP
#define FIXED_DELAY_MODULE_HPP

#include <cstdint>
#include <queue>
#include <utility>
#include <atomic>
#include <chrono>

#include <boost/asio.hpp>

#include "../Module.hpp"
#include "../../utils/Mqtt.hpp"
#include "../../utils/Packet.hpp"

class FixedDelayModule : public Module {
	public:
		FixedDelayModule(boost::asio::io_service &io_service, Mqtt &mqtt, uint64_t delay);

		void setDelay(uint64_t delay);

		void receiveFromLeftModule(std::shared_ptr<Packet> packet);
		void receiveFromRightModule(std::shared_ptr<Packet> packet);
	private:
		Mqtt &mqtt;
		std::atomic<uint64_t> delay;

		ReceivingPort<std::shared_ptr<Packet>> input_port_lr;
		SendingPort<std::shared_ptr<Packet>> output_port_lr;
		boost::asio::high_resolution_timer timer_lr;
		std::queue<std::pair<std::chrono::high_resolution_clock::time_point, std::shared_ptr<Packet>>> packet_queue_lr;
		void setQueueTimeoutLr();
		void processQueueLr(const boost::system::error_code& error);

		ReceivingPort<std::shared_ptr<Packet>> input_port_rl;
		SendingPort<std::shared_ptr<Packet>> output_port_rl;
		boost::asio::high_resolution_timer timer_rl;
		std::queue<std::pair<std::chrono::high_resolution_clock::time_point, std::shared_ptr<Packet>>> packet_queue_rl;
		void setQueueTimeoutRl();
		void processQueueRl(const boost::system::error_code& error);
};

#endif
