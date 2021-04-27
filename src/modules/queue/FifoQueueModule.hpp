#ifndef FIFO_QUEUE_MODULE_HPP
#define FIFO_QUEUE_MODULE_HPP

#include <atomic>
#include <memory>
#include <queue>

#include <boost/asio.hpp>

#include "../Module.hpp"
#include "../../utils/Packet.hpp"

class FifoQueueModule : public Module {
	public:
		FifoQueueModule(boost::asio::io_service &io_service, size_t buffer_size);

		const char* getType() const {
			return "fifo_queue";
		}

	private:
		ReceivingPort<std::shared_ptr<Packet>> input_port;
		RespondingPort<std::shared_ptr<Packet>> output_port;

		Parameter parameter_buffer_size = {100, 0, std::numeric_limits<double>::quiet_NaN(), 1};

		Statistic statistic_queue_length;

		std::queue<std::shared_ptr<Packet>> packet_queue;

		void enqueue(std::shared_ptr<Packet> packet);
		std::shared_ptr<Packet> dequeue();

		boost::asio::high_resolution_timer timer_statistics;
		void statistics(const boost::system::error_code& error);
};

#endif
