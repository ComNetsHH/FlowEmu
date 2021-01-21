#ifndef FIXED_DELAY_MODULE_HPP
#define FIXED_DELAY_MODULE_HPP

#include <vector>
#include <cstdint>
#include <queue>
#include <utility>

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "../Module.hpp"

class FixedDelayModule : public ModuleHasLeft<boost::asio::const_buffer>, public ModuleHasRight<boost::asio::const_buffer> {
	public:
		FixedDelayModule(boost::asio::io_service &io_service, uint64_t delay);

		void processQueue();

		void receiveFromLeftModule(boost::asio::const_buffer packet) override;
		void receiveFromRightModule(boost::asio::const_buffer packet) override;
	private:
		uint64_t delay;

		boost::asio::deadline_timer timer;
		std::queue<std::pair<boost::posix_time::ptime, boost::asio::const_buffer>> packet_queue;
};

#endif
