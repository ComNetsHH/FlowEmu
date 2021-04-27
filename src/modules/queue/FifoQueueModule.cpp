#include "FifoQueueModule.hpp"

#include <boost/bind.hpp>

using namespace std;

FifoQueueModule::FifoQueueModule(boost::asio::io_service &io_service, size_t buffer_size) : timer_statistics(io_service) {
	setName("FIFO Queue");
	addPort({"in", "In", PortInfo::Side::left, &input_port});
	addPort({"out", "Out", PortInfo::Side::right, &output_port});
	addParameter({"buffer_size", "Buffer", "packets", &parameter_buffer_size});
	addStatistic({"queue_length", "Queue", "packets", &statistic_queue_length});

	input_port.setReceiveHandler(bind(&FifoQueueModule::enqueue, this, placeholders::_1));
	output_port.setRequestHandler(bind(&FifoQueueModule::dequeue, this));

	parameter_buffer_size.set(buffer_size);

	// Start statistics timer
	timer_statistics.expires_from_now(chrono::milliseconds(0));
	timer_statistics.async_wait(boost::bind(&FifoQueueModule::statistics, this, boost::asio::placeholders::error));
}

void FifoQueueModule::enqueue(shared_ptr<Packet> packet) {
	if(packet_queue.size() >= parameter_buffer_size.get()) {
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

	statistic_queue_length.set(packet_queue.size());

	timer_statistics.expires_at(timer_statistics.expiry() + chrono::milliseconds(100));
	timer_statistics.async_wait(boost::bind(&FifoQueueModule::statistics, this, boost::asio::placeholders::error));
}
