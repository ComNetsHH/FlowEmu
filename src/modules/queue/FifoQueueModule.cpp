#include "FifoQueueModule.hpp"

#include <boost/bind.hpp>

using namespace std;

FifoQueueModule::FifoQueueModule(boost::asio::io_service &io_service, Mqtt &mqtt, size_t buffer_size) : timer_statistics(io_service), mqtt(mqtt) {
	setName("FIFO Queue");
	addPort({"in", "In", PortInfo::Side::left, &input_port});
	addPort({"out", "Out", PortInfo::Side::right, &output_port});

	input_port.setReceiveHandler(bind(&FifoQueueModule::enqueue, this, placeholders::_1));
	output_port.setRequestHandler(bind(&FifoQueueModule::dequeue, this));

	setBufferSize(buffer_size);

	mqtt.subscribe("set/fifo_queue/buffer_size", [&](const string &topic, const string &message) {
		size_t buffer_size = stoul(message);
		setBufferSize(buffer_size);
	});

	// Start statistics timer
	timer_statistics.expires_from_now(chrono::milliseconds(0));
	timer_statistics.async_wait(boost::bind(&FifoQueueModule::statistics, this, boost::asio::placeholders::error));
}

void FifoQueueModule::setBufferSize(size_t buffer_size) {
	this->buffer_size = buffer_size;

	mqtt.publish("get/fifo_queue/buffer_size", to_string(buffer_size), true);
}

void FifoQueueModule::enqueue(shared_ptr<Packet> packet) {
	if(packet_queue.size() >= buffer_size) {
		return;
	}

	packet_queue.emplace(packet);

	output_port.notify();
}

std::shared_ptr<Packet> FifoQueueModule::dequeue() {
	shared_ptr<Packet> packet;
	if(!packet_queue.empty()) {
		packet = packet_queue.front();
		packet_queue.pop();
	}

	return packet;
}

void FifoQueueModule::statistics(const boost::system::error_code& error) {
	if(error == boost::asio::error::operation_aborted) {
		return;
	}

	mqtt.publish("get/fifo_queue/queue_length", to_string(packet_queue.size()), true);

	timer_statistics.expires_at(timer_statistics.expiry() + chrono::milliseconds(100));
	timer_statistics.async_wait(boost::bind(&FifoQueueModule::statistics, this, boost::asio::placeholders::error));
}
