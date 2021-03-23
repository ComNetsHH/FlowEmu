#include "FifoQueueModule.hpp"

using namespace std;

FifoQueueModule::FifoQueueModule(Mqtt &mqtt, size_t buffer_size) : mqtt(mqtt) {
	setName("FIFO Queue");
	addPort({"in", "In", PortInfo::Side::left, &input_port});
	addPort({"out", "Out", PortInfo::Side::right, &output_port});

	input_port.setReceiveHandler(bind(&FifoQueueModule::push, this, placeholders::_1));
	output_port.setRequestHandler(bind(&FifoQueueModule::pop, this));

	setBufferSize(buffer_size);

	mqtt.subscribe("set/fifo_queue/buffer_size", [&](const string &topic, const string &message) {
		size_t buffer_size = stoul(message);
		setBufferSize(buffer_size);
	});
}

void FifoQueueModule::setBufferSize(size_t buffer_size) {
	this->buffer_size = buffer_size;

	mqtt.publish("get/fifo_queue/buffer_size", to_string(buffer_size), true);
}

void FifoQueueModule::push(shared_ptr<Packet> packet) {
	if(packet_queue.size() >= buffer_size) {
		return;
	}

	packet_queue.emplace(packet);
}

std::shared_ptr<Packet> FifoQueueModule::pop() {
	shared_ptr<Packet> packet;
	if(!packet_queue.empty()) {
		packet = packet_queue.front();
		packet_queue.pop();
	}

	return packet;
}
