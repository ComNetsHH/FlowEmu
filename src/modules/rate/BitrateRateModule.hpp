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

class BitrateRateModule : public Module {
	public:
		BitrateRateModule(boost::asio::io_service &io_service, Mqtt &mqtt, uint64_t bitrate);
		~BitrateRateModule();

		const char* getType() const {
			return "bitrate_rate";
		}

		void setBitrate(uint64_t bitrate);
	private:
		Mqtt &mqtt;

		std::atomic<uint64_t> bitrate;

		RequestingPort<std::shared_ptr<Packet>> input_port;
		SendingPort<std::shared_ptr<Packet>> output_port;

		boost::asio::high_resolution_timer timer;
		bool active = false;
		void process(const boost::system::error_code& error);
};

#endif
