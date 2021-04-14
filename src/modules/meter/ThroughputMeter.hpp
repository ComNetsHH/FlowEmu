#ifndef THROUGHPUT_METER_MODULE_HPP
#define THROUGHPUT_METER_MODULE_HPP

#include <queue>
#include <utility>
#include <memory>
#include <string>

#include <boost/asio.hpp>

#include "../Module.hpp"
#include "../../utils/Mqtt.hpp"
#include "../../utils/Packet.hpp"

class ThroughputMeter : public Module {
	public:
		ThroughputMeter(boost::asio::io_service &io_service, Mqtt &mqtt, const std::string &module_id);
		~ThroughputMeter();

		const char* getType() const {
			return "throughput_meter";
		}

		void receive(std::shared_ptr<Packet> packet);

	private:
		Mqtt &mqtt;
		std::string module_id;

		ReceivingPort<std::shared_ptr<Packet>> input_port;
		SendingPort<std::shared_ptr<Packet>> output_port;

		boost::asio::high_resolution_timer timer;
		uint64_t bytes_sum = 0;
		std::queue<std::pair<std::chrono::high_resolution_clock::time_point, uint64_t>> bytes;
		void process(const boost::system::error_code& error);
};

#endif
