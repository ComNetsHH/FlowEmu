#ifndef FIXED_INTERVAL_RATE_MODULE_HPP
#define FIXED_INTERVAL_RATE_MODULE_HPP

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>

#include <boost/asio.hpp>

#include "../Module.hpp"
#include "../../utils/Packet.hpp"

class FixedIntervalRateModule : public Module {
	public:
		FixedIntervalRateModule(boost::asio::io_service &io_service, std::chrono::high_resolution_clock::duration interval);
		~FixedIntervalRateModule();

		const char* getType() const {
			return "fixed_interval_rate";
		}

	private:
		RequestingPort<std::shared_ptr<Packet>> input_port_lr;
		SendingPort<std::shared_ptr<Packet>> output_port_lr;
		RequestingPort<std::shared_ptr<Packet>> input_port_rl;
		SendingPort<std::shared_ptr<Packet>> output_port_rl;

		Parameter parameter_interval = {1, 0, std::numeric_limits<double>::quiet_NaN(), 1};
		Parameter parameter_rate = {1000, 0, std::numeric_limits<double>::quiet_NaN(), 1};

		boost::asio::high_resolution_timer timer_lr;
		void processLr(const boost::system::error_code& error);

		boost::asio::high_resolution_timer timer_rl;
		void processRl(const boost::system::error_code& error);
};

#endif
