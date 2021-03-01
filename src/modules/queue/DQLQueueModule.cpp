#include "DQLQueueModule.hpp"

#include <boost/bind.hpp>

using namespace std;

DQLQueueModule::DQLQueueModule(boost::asio::io_service &io_service, Mqtt &mqtt, chrono::high_resolution_clock::duration interval) : timer_lr(io_service), timer_statistics(io_service), mqtt(mqtt), deep_q_learning(10, 3, 2, 0, 1, 1000000UL, false, "ml_models/graph.meta", "ml_models/graph") {
	setInterval(chrono::nanoseconds(interval));
	setBufferSize(100);
	setEpsilon(0.001);

	// MQTT
	mqtt.subscribe("set/dql_queue/interval", [&](const string &topic, const string &message) {
		uint64_t interval = stoul(message);
		setInterval(chrono::nanoseconds(interval));
	});

	mqtt.subscribe("set/dql_queue/rate", [&](const string &topic, const string &message) {
		uint64_t rate = stoul(message);
		setRate(rate);
	});

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

	// Start timer
	chrono::high_resolution_clock::time_point now = chrono::high_resolution_clock::now();

	timer_lr.expires_at(now);
	timer_lr.async_wait(boost::bind(&DQLQueueModule::processLr, this, boost::asio::placeholders::error));

	timer_statistics.expires_at(now);
	timer_statistics.async_wait(boost::bind(&DQLQueueModule::statistics, this, boost::asio::placeholders::error));
}

void DQLQueueModule::setInterval(chrono::high_resolution_clock::duration interval) {
	this->interval = interval;

	mqtt.publish("get/dql_queue/interval", to_string(chrono::nanoseconds(interval).count()), true);
	mqtt.publish("get/dql_queue/rate", to_string(1000000000 / chrono::nanoseconds(interval).count()), true);
}

void DQLQueueModule::setRate(uint64_t rate) {
	this->interval = chrono::nanoseconds(1000000000 / rate);

	mqtt.publish("get/dql_queue/interval", to_string(chrono::nanoseconds(interval).count()), true);
	mqtt.publish("get/dql_queue/rate", to_string(1000000000 / chrono::nanoseconds(interval).count()), true);
}

void DQLQueueModule::setBufferSize(size_t buffer_size) {
	this->buffer_size = buffer_size;

	mqtt.publish("get/dql_queue/buffer_size", to_string(buffer_size), true);
}

void DQLQueueModule::setEpsilon(double epsilon) {
	this->epsilon = epsilon;

	mqtt.publish("get/dql_queue/epsilon", to_string(epsilon), true);
}

void DQLQueueModule::receiveFromLeftModule(shared_ptr<Packet> packet) {
	if(packet_queue_lr.size() < buffer_size.load()) {
		packet_queue_lr.push(packet);
	}
}

void DQLQueueModule::processLr(const boost::system::error_code& error) {
	if(error == boost::asio::error::operation_aborted) {
		return;
	}

	auto observation = observation_tf.tensor<float, 2>();
	auto observation_new = observation_new_tf.tensor<float, 2>();

	// Get prediction
	tensorflow::int64 action = deep_q_learning.getPrediction(observation_tf, epsilon.load());

	// Drop packet
	if(action == 0 && !packet_queue_lr.empty()) {
		packet_queue_lr.pop();
	}

	// Forward packet
	bool packet_sent = 0;
	if(!packet_queue_lr.empty()) {
		passToRightModule(packet_queue_lr.front());
		packet_queue_lr.pop();
		packet_sent = 1;
	}

	// Get observation
	size_t queue_length = packet_queue_lr.size();
	for(int y = 0; y < 10; y++) {
		double a = pow(0.1, y);
		observation_new(y, 0) = (1.0 - a) * observation(y, 0) + a * (1.0 - ((double) queue_length / 100.0));
		observation_new(y, 1) = (1.0 - a) * observation(y, 1) + a * (double) packet_sent;
		observation_new(y, 2) = (1.0 - a) * observation(y, 2) + a * (double) action;
	}

	// Add experience
	reward = action ? 127 * observation_new(2, 1) * observation_new(2, 0) : -128;
	deep_q_learning.addExperience(observation_tf, action, reward, observation_new_tf);

	// Update observation
	observation = observation_new;

	// Set timer
	timer_lr.expires_at(timer_lr.expiry() + interval.load());
	timer_lr.async_wait(boost::bind(&DQLQueueModule::processLr, this, boost::asio::placeholders::error));
}

void DQLQueueModule::receiveFromRightModule(shared_ptr<Packet> packet) {
	passToLeftModule(packet);
}

void DQLQueueModule::statistics(const boost::system::error_code& error) {
	if(error == boost::asio::error::operation_aborted) {
		return;
	}

	auto observation = observation_tf.tensor<float, 2>();
	for(int x = 0; x < 3; x++) {
		for(int y = 0; y < 10; y++) {
			cout << observation(y, x) << " ";
		}
		cout << endl;
	}
	cout << "Reward: " << (int) reward << endl;
	cout << "-------------------------" << endl;

	timer_statistics.expires_at(timer_statistics.expiry() + chrono::seconds(1));
	timer_statistics.async_wait(boost::bind(&DQLQueueModule::statistics, this, boost::asio::placeholders::error));
}

DQLQueueModule::~DQLQueueModule() {
	running = false;
	training_thread->join();
}
