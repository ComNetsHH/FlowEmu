#ifndef DQL_QUEUE_MODULE_HPP
#define DQL_QUEUE_MODULE_HPP

#include <vector>
#include <cstdint>
#include <queue>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>

#include <boost/asio.hpp>

#include "../Module.hpp"
#include "../../ml/DeepQLearning.hpp"
#include "../../utils/Packet.hpp"

class DQLQueueModule : public Module {
	public:
		DQLQueueModule(boost::asio::io_service &io_service, size_t buffer_size, double epsilon);
		~DQLQueueModule();

		const char* getType() const {
			return "dql_queue";
		}

	private:
		ReceivingPort<std::shared_ptr<Packet>> input_port;
		RespondingPort<std::shared_ptr<Packet>> output_port;

		Parameter parameter_buffer_size = {100, 0, std::numeric_limits<double>::quiet_NaN(), 1};
		Parameter parameter_epsilon = {0.001, 0, 1, 0.001};

		Statistic statistic_queue_length;

		DeepQLearning deep_q_learning;

		tensorflow::Tensor observation_tf;
		tensorflow::Tensor observation_new_tf;
		int8_t reward;

		std::unique_ptr<std::thread> training_thread;
		std::atomic<bool> running;

		std::queue<std::shared_ptr<Packet>> packet_queue;

		void enqueue(std::shared_ptr<Packet> packet);
		std::shared_ptr<Packet> dequeue();

		boost::asio::high_resolution_timer timer_statistics;
		void statistics(const boost::system::error_code& error);
};

#endif
