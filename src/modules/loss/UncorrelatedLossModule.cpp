#include "UncorrelatedLossModule.hpp"

using namespace std;

UncorrelatedLossModule::UncorrelatedLossModule(Mqtt &mqtt, double p, uint32_t seed) : mqtt(mqtt) {
	setLossProbability(p);
	setSeed(seed);

	mqtt.subscribe("set/uncorrelated_loss/loss_probability", [&](const string &topic, const string &message) {
		double loss = stod(message);
		setLossProbability(loss);
	});

	mqtt.subscribe("set/uncorrelated_loss/seed", [&](const string &topic, const string &message) {
		uint32_t seed = stoul(message);
		setSeed(seed);
	});
}

void UncorrelatedLossModule::setLossProbability(double p) {
	this->p = p;

	distribution.reset(new bernoulli_distribution(p));

	mqtt.publish("get/uncorrelated_loss/loss_probability", to_string(p), true);
}

void UncorrelatedLossModule::setSeed(uint32_t seed) {
	this->seed = seed;

	generator_loss.seed(seed);

	mqtt.publish("get/uncorrelated_loss/seed", to_string(seed), true);
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
