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
#include "../../utils/Mqtt.hpp"
#include "../../utils/Packet.hpp"

class DQLQueueModule : public ModuleHasLeft<std::shared_ptr<Packet>>, public ModuleHasRight<std::shared_ptr<Packet>> {
	public:
		DQLQueueModule(boost::asio::io_service &io_service, Mqtt &mqtt, std::chrono::high_resolution_clock::duration interval);
		~DQLQueueModule();

		void setInterval(std::chrono::high_resolution_clock::duration interval);
		void setRate(uint64_t rate);
		void setBufferSize(size_t buffer_size);
		void setEpsilon(double epsilon);

		void receiveFromLeftModule(std::shared_ptr<Packet> packet) override;
		void receiveFromRightModule(std::shared_ptr<Packet> packet) override;
	private:
		Mqtt &mqtt;
		DeepQLearning deep_q_learning;

		std::atomic<std::chrono::high_resolution_clock::duration> interval;
		std::atomic<size_t> buffer_size;
		std::atomic<double> epsilon;

		tensorflow::Tensor observation_tf;
		tensorflow::Tensor observation_new_tf;
		int8_t reward;

		std::unique_ptr<std::thread> training_thread;
		std::atomic<bool> running;

		boost::asio::high_resolution_timer timer_lr;
		std::queue<std::shared_ptr<Packet>> packet_queue_lr;
		void processLr(const boost::system::error_code& error);

		boost::asio::high_resolution_timer timer_statistics;
		void statistics(const boost::system::error_code& error);
};

#endif
