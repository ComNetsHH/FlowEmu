#ifndef FIFO_QUEUE_MODULE_HPP
#define FIFO_QUEUE_MODULE_HPP

#include <atomic>
#include <memory>
#include <queue>

#include "../Module.hpp"
#include "../../utils/Mqtt.hpp"
#include "../../utils/Packet.hpp"

class FifoQueueModule : public Module {
	public:
		FifoQueueModule(Mqtt &mqtt, size_t buffer_size);

		void setBufferSize(size_t buffer_size);

	private:
		Mqtt &mqtt;

		std::atomic<size_t> buffer_size;

		ReceivingPort<std::shared_ptr<Packet>> input_port;
		RespondingPort<std::shared_ptr<Packet>> output_port;

		std::queue<std::shared_ptr<Packet>> packet_queue;

		void enqueue(std::shared_ptr<Packet> packet);
		std::shared_ptr<Packet> dequeue();
};

#endif
