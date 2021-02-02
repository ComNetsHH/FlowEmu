#include "GilbertElliotLossModule.hpp"

//#include <iostream>

#include <boost/bind.hpp>

using namespace std;

GilbertElliotLossModule::GilbertElliotLossModule(boost::asio::io_service &io_service, Mqtt &mqtt, double p01, double p10, double e0, double e1, uint32_t seed_transition, uint32_t seed_loss) : timer_transition(io_service), mqtt(mqtt) {
	setModelParameters(p01, p10, e0, e1);
	setSeedTransition(seed_transition);
	setSeedLoss(seed_loss);

	mqtt.subscribe("set/gilbert_elliot_loss/model_parameters", [&](const string &topic, const Json::Value &json_root) {
		double p01 = json_root.get("p01", 0).asDouble();
		double p10 = json_root.get("p10", 0).asDouble();
		double e0 = json_root.get("e0", 0).asDouble();
		double e1 = json_root.get("e1", 1).asDouble();
		setModelParameters(p01, p10, e0, e1);
	});

	mqtt.subscribe("set/gilbert_elliot_loss/seed_transition", [&](const string &topic, const string &message) {
		uint32_t seed_transition = stoul(message);
		setSeedTransition(seed_transition);
	});

	mqtt.subscribe("set/gilbert_elliot_loss/seed_loss", [&](const string &topic, const string &message) {
		uint32_t seed_loss = stoul(message);
		setSeedLoss(seed_loss);
	});

	mqtt.subscribe("set/gilbert_elliot_loss", [&](const string &topic, const string &message) {
		if(message == "reset") {
			reset();
		}
	});

	reset();
}

void GilbertElliotLossModule::setModelParameters(double p01, double p10, double e0, double e1) {
	this->p01 = p01;
	this->p10 = p10;
	this->e0 = e0;
	this->e1 = e1;

	distribution_p01.reset(new exponential_distribution<double>(p01));
	distribution_p10.reset(new exponential_distribution<double>(p10));
	distribution_e0.reset(new bernoulli_distribution(e0));
	distribution_e1.reset(new bernoulli_distribution(e1));

	Json::Value model_parameters;
	model_parameters["p01"] = p01;
	model_parameters["p10"] = p10;
	model_parameters["e0"] = e0;
	model_parameters["e1"] = e1;
	mqtt.publish("get/gilbert_elliot_loss/model_parameters", model_parameters, true);
}

void GilbertElliotLossModule::setSeedTransition(uint32_t seed_transition) {
	this->seed_transition = seed_transition;

	generator_transition.seed(seed_transition);

	mqtt.publish("get/gilbert_elliot_loss/seed_transition", to_string(seed_transition), true);
}

void GilbertElliotLossModule::setSeedLoss(uint32_t seed_loss) {
	this->seed_loss = seed_loss;

	generator_loss.seed(seed_loss);

	mqtt.publish("get/gilbert_elliot_loss/seed_loss", to_string(seed_loss), true);
}

void GilbertElliotLossModule::reset() {
	timer_transition.cancel();

	generator_transition.seed(seed_transition);
	generator_loss.seed(seed_loss);
	state = 0;

	chrono::high_resolution_clock::duration sojourn_time = chrono::nanoseconds((uint64_t) ((*distribution_p01)(generator_transition) * 1000000));
	//cout << "Gilbert-Elliot model: Stay in state 0 with loss probability " << e0 << " for " << (double) sojourn_time.count() / 1000000 << "ms" << endl;
	timer_transition.expires_from_now(sojourn_time);
	timer_transition.async_wait(boost::bind(&GilbertElliotLossModule::transition, this, boost::asio::placeholders::error));

	mqtt.publish("get/gilbert_elliot_loss/state", to_string(state), true);
}

void GilbertElliotLossModule::transition(const boost::system::error_code& error) {
	if(error == boost::asio::error::operation_aborted) {
		return;
	}

	if(state == 0) {
		state = 1;

		chrono::high_resolution_clock::duration sojourn_time = chrono::nanoseconds((uint64_t) ((*distribution_p10)(generator_transition) * 1000000));
		//cout << "Gilbert-Elliot model: Stay in state 1 with loss probability " << e1 << " for " << (double) sojourn_time.count() / 1000000 << "ms" << endl;
		timer_transition.expires_at(timer_transition.expiry() + sojourn_time);
	} else {
		state = 0;

		chrono::high_resolution_clock::duration sojourn_time = chrono::nanoseconds((uint64_t) ((*distribution_p01)(generator_transition) * 1000000));
		//cout << "Gilbert-Elliot model: Stay in state 0 with loss probability " << e0 << " for " << (double) sojourn_time.count() / 1000000 << "ms" << endl;
		timer_transition.expires_at(timer_transition.expiry() + sojourn_time);
	}

	timer_transition.async_wait(boost::bind(&GilbertElliotLossModule::transition, this, boost::asio::placeholders::error));

	mqtt.publish("get/gilbert_elliot_loss/state", to_string(state), true);
}

bool GilbertElliotLossModule::isLost() {
	if(state == 0) {
		return (*distribution_e0)(generator_loss);
	} else {
		return (*distribution_e1)(generator_loss);
	}
}

void GilbertElliotLossModule::receiveFromLeftModule(shared_ptr<Packet> packet) {
	if(!isLost()) {
		passToRightModule(packet);
	}
}

void GilbertElliotLossModule::receiveFromRightModule(shared_ptr<Packet> packet) {
	if(!isLost()) {
		passToLeftModule(packet);
	}
}
