#include "UncorrelatedLossModule.hpp"

using namespace std;

UncorrelatedLossModule::UncorrelatedLossModule(Mqtt &mqtt, double p, uint32_t seed_loss) : generator_loss(seed_loss), mqtt(mqtt) {
	setLossProbability(p);

	mqtt.subscribe("set/uncorrelated_loss/loss_probability", [&](const string &topic, const string &message) {
		double loss = stod(message);
		setLossProbability(loss);
	});
}

void UncorrelatedLossModule::setLossProbability(double p) {
	this->p = p;

	distribution.reset(new bernoulli_distribution(p));

	mqtt.publish("get/uncorrelated_loss/loss_probability", to_string(p), true);
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
