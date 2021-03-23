#ifndef TRACE_RATE_MODULE_HPP
#define TRACE_RATE_MODULE_HPP

#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

#include <boost/asio.hpp>

#include "../Module.hpp"
#include "../../utils/Mqtt.hpp"
#include "../../utils/Packet.hpp"

class TraceRateModule : public Module {
	public:
		TraceRateModule(boost::asio::io_service &io_service, Mqtt &mqtt, const std::string &downlink_trace_filename, const std::string &up_trace_filename);
		~TraceRateModule();

		void reset();

	private:
		Mqtt &mqtt;

		std::chrono::high_resolution_clock::time_point trace_start;

		RequestingPort<std::shared_ptr<Packet>> input_port_lr;
		SendingPort<std::shared_ptr<Packet>> output_port_lr;
		std::vector<uint32_t> trace_lr;
		std::vector<uint32_t>::iterator trace_lr_itr;
		boost::asio::high_resolution_timer timer_lr;
		void processLr(const boost::system::error_code& error);

		RequestingPort<std::shared_ptr<Packet>> input_port_rl;
		SendingPort<std::shared_ptr<Packet>> output_port_rl;
		std::vector<uint32_t> trace_rl;
		std::vector<uint32_t>::iterator trace_rl_itr;
		boost::asio::high_resolution_timer timer_rl;
		void processRl(const boost::system::error_code& error);

		void loadTrace(std::vector<uint32_t> &trace, const std::string &trace_filename);
};

#endif
