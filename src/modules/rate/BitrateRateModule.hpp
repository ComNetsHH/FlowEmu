#ifndef BITRATE_RATE_MODULE_HPP
#define BITRATE_RATE_MODULE_HPP

#include <cstdint>
#include <queue>
#include <utility>
#include <atomic>
#include <chrono>

#include <boost/asio.hpp>

#include "../Module.hpp"
#include "../../utils/Packet.hpp"

class BitrateRateModule : public Module {
	public:
		BitrateRateModule(boost::asio::io_service &io_service, uint64_t bitrate);
		~BitrateRateModule();

		const char* getType() const {
			return "bitrate_rate";
		}

	private:
		RequestingPort<std::shared_ptr<Packet>> input_port;
		SendingPort<std::shared_ptr<Packet>> output_port;

		Parameter parameter_bitrate = {1000000, 0, std::numeric_limits<double>::quiet_NaN(), 1000};

		boost::asio::high_resolution_timer timer;
		bool active = false;
		void process(const boost::system::error_code& error);
};

#endif
