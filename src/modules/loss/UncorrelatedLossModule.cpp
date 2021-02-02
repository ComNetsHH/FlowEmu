#include "UncorrelatedLossModule.hpp"

using namespace std;

UncorrelatedLossModule::UncorrelatedLossModule(Mqtt &mqtt, double p, uint32_t loss_seed) : generator_loss(), mqtt(mqtt) {
	setLossProbability(p);
	setLossSeed(loss_seed);

	mqtt.subscribe("set/uncorrelated_loss/loss_probability", [&](const string &topic, const string &message) {
		double loss = stod(message);
		setLossProbability(loss);
	});

	mqtt.subscribe("set/uncorrelated_loss/loss_seed", [&](const string &topic, const string &message) {
		uint32_t loss_seed = stoul(message);
		setLossSeed(loss_seed);
	});
}

void UncorrelatedLossModule::setLossProbability(double p) {
	this->p = p;

	distribution.reset(new bernoulli_distribution(p));

	mqtt.publish("get/uncorrelated_loss/loss_probability", to_string(p), true);
}

void UncorrelatedLossModule::setLossSeed(uint32_t loss_seed) {
	this->loss_seed = loss_seed;

	generator_loss.seed(loss_seed);

	mqtt.publish("get/uncorrelated_loss/loss_seed", to_string(loss_seed), true);
}

void UncorrelatedLossModule::receiveFromLeftModule(shared_ptr<Packet> packet) {
	if(!(*distribution)(generator_loss)) {
		passToRightModule(packet);
	}
}

void UncorrelatedLossModule::receiveFromRightModule(shared_ptr<Packet> packet) {
	if(!(*distribution)(generator_loss)) {
		passToLeftModule(packet);
	}
}
