#ifndef FIXED_INTERVAL_RATE_MODULE_HPP
#define FIXED_INTERVAL_RATE_MODULE_HPP

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>

#include <boost/asio.hpp>

#include "../Module.hpp"
#include "../../utils/Mqtt.hpp"
#include "../../utils/Packet.hpp"

class FixedIntervalRateModule : public Module {
	public:
		FixedIntervalRateModule(boost::asio::io_service &io_service, Mqtt &mqtt, std::chrono::high_resolution_clock::duration interval);
		~FixedIntervalRateModule();

		const char* getType() const {
			return "fixed_interval_rate";
		}

		void setInterval(std::chrono::high_resolution_clock::duration interval);
		void setRate(uint64_t rate);

	private:
		Mqtt &mqtt;

		std::atomic<std::chrono::high_resolution_clock::duration> interval;

		RequestingPort<std::shared_ptr<Packet>> input_port_lr;
		SendingPort<std::shared_ptr<Packet>> output_port_lr;
		boost::asio::high_resolution_timer timer_lr;
		void processLr(const boost::system::error_code& error);

		RequestingPort<std::shared_ptr<Packet>> input_port_rl;
		SendingPort<std::shared_ptr<Packet>> output_port_rl;
		boost::asio::high_resolution_timer timer_rl;
		void processRl(const boost::system::error_code& error);
};

#endif
