#ifndef TRACE_RATE_MODULE_HPP
#define TRACE_RATE_MODULE_HPP

#include <vector>
#include <cstdint>
#include <queue>
#include <memory>
#include <chrono>

#include <boost/asio.hpp>

#include "../Module.hpp"
#include "../../utils/Mqtt.hpp"
#include "../../utils/Packet.hpp"

class TraceRateModule : public ModuleHasLeft<std::shared_ptr<Packet>>, public ModuleHasRight<std::shared_ptr<Packet>> {
	public:
		TraceRateModule(boost::asio::io_service &io_service, Mqtt &mqtt, std::string downlink_trace_filename, std::string up_trace_filename, size_t buffer_size);

		void setBufferSize(size_t buffer_size);

		void reset();

		void receiveFromLeftModule(std::shared_ptr<Packet> packet) override;
		void receiveFromRightModule(std::shared_ptr<Packet> packet) override;

	private:
		Mqtt &mqtt;

		std::atomic<size_t> buffer_size;

		std::vector<uint32_t> trace_lr;
		std::vector<uint32_t>::iterator trace_lr_itr;
		std::chrono::high_resolution_clock::time_point trace_lr_start;
		boost::asio::high_resolution_timer timer_lr;
		std::queue<std::shared_ptr<Packet>> packet_queue_lr;
		void processLr(const boost::system::error_code& error);

		std::vector<uint32_t> trace_rl;
		std::vector<uint32_t>::iterator trace_rl_itr;
		std::chrono::high_resolution_clock::time_point trace_rl_start;
		boost::asio::high_resolution_timer timer_rl;
		std::queue<std::shared_ptr<Packet>> packet_queue_rl;
		void processRl(const boost::system::error_code& error);

		void loadTrace(std::vector<uint32_t> &trace, const std::string &trace_filename);
};

#endif
