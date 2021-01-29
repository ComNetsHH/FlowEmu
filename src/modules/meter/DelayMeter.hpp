#ifndef DELAY_METER_MODULE_HPP
#define DELAY_METER_MODULE_HPP

#include <deque>
#include <utility>
#include <memory>

#include <boost/asio.hpp>

#include "../Module.hpp"
#include "../../utils/Packet.hpp"

class DelayMeter : public ModuleHasLeft<std::shared_ptr<Packet>>, public ModuleHasRight<std::shared_ptr<Packet>> {
	public:
		DelayMeter(boost::asio::io_service &io_service);

		void receiveFromLeftModule(std::shared_ptr<Packet> packet) override;
		void receiveFromRightModule(std::shared_ptr<Packet> packet) override;

	private:
		boost::asio::high_resolution_timer timer;
		std::deque<std::pair<std::chrono::high_resolution_clock::time_point, std::chrono::high_resolution_clock::time_point>> creation_time_points;
		void process(const boost::system::error_code& error);
};

#endif
