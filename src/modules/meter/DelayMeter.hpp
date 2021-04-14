#ifndef DELAY_METER_MODULE_HPP
#define DELAY_METER_MODULE_HPP

#include <deque>
#include <utility>
#include <memory>

#include <boost/asio.hpp>

#include "../Module.hpp"
#include "../../utils/Mqtt.hpp"
#include "../../utils/Packet.hpp"

class DelayMeter : public Module {
	public:
		DelayMeter(boost::asio::io_service &io_service, Mqtt &mqtt);
		~DelayMeter();

		const char* getType() const {
			return "delay_meter";
		}

		void receive(std::shared_ptr<Packet> packet);

	private:
		Mqtt &mqtt;

		ReceivingPort<std::shared_ptr<Packet>> input_port;
		SendingPort<std::shared_ptr<Packet>> output_port;

		boost::asio::high_resolution_timer timer;
		std::deque<std::pair<std::chrono::high_resolution_clock::time_point, std::chrono::high_resolution_clock::time_point>> creation_time_points;
		void process(const boost::system::error_code& error);
};

#endif
