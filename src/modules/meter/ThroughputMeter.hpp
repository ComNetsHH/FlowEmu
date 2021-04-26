#ifndef THROUGHPUT_METER_MODULE_HPP
#define THROUGHPUT_METER_MODULE_HPP

#include <queue>
#include <utility>
#include <memory>
#include <string>

#include <boost/asio.hpp>

#include "../Module.hpp"
#include "../../utils/Packet.hpp"

class ThroughputMeter : public Module {
	public:
		ThroughputMeter(boost::asio::io_service &io_service);
		~ThroughputMeter();

		const char* getType() const {
			return "throughput_meter";
		}

	private:
		ReceivingPort<std::shared_ptr<Packet>> input_port;
		SendingPort<std::shared_ptr<Packet>> output_port;

		Statistic statistic_bytes_per_second;
		Statistic statistic_packets_per_second;

		void receive(std::shared_ptr<Packet> packet);

		boost::asio::high_resolution_timer timer;
		uint64_t bytes_sum = 0;
		std::queue<std::pair<std::chrono::high_resolution_clock::time_point, uint64_t>> bytes;
		void process(const boost::system::error_code& error);
};

#endif
