#include "DQLQueueModule.hpp"

#include <boost/bind.hpp>

using namespace std;

DQLQueueModule::DQLQueueModule(boost::asio::io_service &io_service, Mqtt &mqtt, size_t buffer_size, double epsilon) : timer_statistics(io_service), mqtt(mqtt), deep_q_learning(10, 3, 2, 0, 1, 1000000UL, false, "ml_models/graph.meta", "ml_models/graph") {
	setName("DQL Queue");
	addPort({"in", "In", PortInfo::Side::left, &input_port});
	addPort({"out", "Out", PortInfo::Side::right, &output_port});

	input_port.setReceiveHandler(bind(&DQLQueueModule::enqueue, this, placeholders::_1));
	output_port.setRequestHandler(bind(&DQLQueueModule::dequeue, this));

	setBufferSize(buffer_size);
	setEpsilon(epsilon);

	// MQTT
	mqtt.subscribe("set/dql_queue/buffer_size", [&](const string &topic, const string &message) {
		size_t buffer_size = stoul(message);
		setBufferSize(buffer_size);
	});

	mqtt.subscribe("set/dql_queue/epsilon", [&](const string &topic, const string &message) {
		double epsilon = stod(message);
		setEpsilon(epsilon);
	});

	// Create observation matrices
	tensorflow::TensorShape observation_shape = tensorflow::TensorShape({10, 3});
	observation_tf = tensorflow::Tensor(tensorflow::DT_FLOAT, observation_shape);
	observation_tf.tensor<float, 2>().setZero();
	observation_new_tf = tensorflow::Tensor(tensorflow::DT_FLOAT, observation_shape);
	observation_new_tf.tensor<float, 2>().setZero();

	// Create training thread
	running = true;
	training_thread.reset(new thread([&]() {
		while(running) {
			bool trained = deep_q_learning.train(0.99, 100);

			if(!trained) {
				this_thread::sleep_for(chrono::seconds(1));
			}
		}
	}));

	// Start statistics timer
	timer_statistics.expires_from_now(chrono::milliseconds(0));
	timer_statistics.async_wait(boost::bind(&DQLQueueModule::statistics, this, boost::asio::placeholders::error));
}

void DQLQueueModule::setBufferSize(size_t buffer_size) {
	this->buffer_size = buffer_size;

	mqtt.publish("get/dql_queue/buffer_size", to_string(buffer_size), true);
}

void DQLQueueModule::setEpsilon(double epsilon) {
	this->epsilon = epsilon;

	mqtt.publish("get/dql_queue/epsilon", to_string(epsilon), true);
}

void DQLQueueModule::enqueue(shared_ptr<Packet> packet) {
	if(packet_queue.size() >= buffer_size) {
		return;
	}

	packet_queue.emplace(packet);

	output_port.notify();
}

shared_ptr<Packet> DQLQueueModule::dequeue() {
	auto observation = observation_tf.tensor<float, 2>();
	auto observation_new = observation_new_tf.tensor<float, 2>();

	// Get prediction
	tensorflow::int64 action = deep_q_learning.getPrediction(observation_tf, epsilon.load());

	// Drop packet
	if(action == 0 && !packet_queue.empty()) {
		packet_queue.pop();
	}

	// Get packet
	shared_ptr<Packet> packet;
	bool packet_sent = 0;
	if(!packet_queue.empty()) {
		packet = packet_queue.front();
		packet_queue.pop();
		packet_sent = 1;
	}

	// Get observation
	size_t queue_length = packet_queue.size();
	for(int y = 0; y < 10; y++) {
		double a = pow(0.1, y);
		observation_new(y, 0) = (1.0 - a) * observation(y, 0) + a * (1.0 - ((double) queue_length / buffer_size));
		observation_new(y, 1) = (1.0 - a) * observation(y, 1) + a * (double) packet_sent;
		observation_new(y, 2) = (1.0 - a) * observation(y, 2) + a * (double) action;
	}

	// Add experience
	reward = action ? 127 * observation_new(2, 1) * observation_new(2, 0) : -128;
	deep_q_learning.addExperience(observation_tf, action, reward, observation_new_tf);

	// Update observation
	observation = observation_new;

	// Return packet
	return packet;
}

void DQLQueueModule::statistics(const boost::system::error_code& error) {
	if(error == boost::asio::error::operation_aborted) {
		return;
	}

	mqtt.publish("get/dql_queue/queue_length", to_string(packet_queue.size()), true);

	auto observation = observation_tf.tensor<float, 2>();
	for(int x = 0; x < 3; x++) {
		for(int y = 0; y < 10; y++) {
			cout << observation(y, x) << " ";
		}
		cout << endl;
	}
	cout << "Reward: " << (int) reward << endl;
	cout << "-------------------------" << endl;

	timer_statistics.expires_at(timer_statistics.expiry() + chrono::milliseconds(100));
	timer_statistics.async_wait(boost::bind(&DQLQueueModule::statistics, this, boost::asio::placeholders::error));
}

DQLQueueModule::~DQLQueueModule() {
	timer_statistics.cancel();

	running = false;
	training_thread->join();
}
