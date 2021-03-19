#include "UncorrelatedLossModule.hpp"

using namespace std;

UncorrelatedLossModule::UncorrelatedLossModule(Mqtt &mqtt, double p, uint32_t seed) : mqtt(mqtt) {
	setName("Fixed Delay");
	addPort({"lr_in", "In", PortInfo::Side::left, &input_port_lr});
	addPort({"lr_out", "Out", PortInfo::Side::right, &output_port_lr});
	addPort({"rl_in", "In", PortInfo::Side::right, &input_port_rl});
	addPort({"rl_out", "Out", PortInfo::Side::left, &output_port_rl});

	input_port_lr.setReceiveHandler(bind(&UncorrelatedLossModule::receiveFromLeftModule, this, placeholders::_1));
	input_port_rl.setReceiveHandler(bind(&UncorrelatedLossModule::receiveFromRightModule, this, placeholders::_1));

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
		output_port_lr.send(packet);
	}
}

void UncorrelatedLossModule::receiveFromRightModule(shared_ptr<Packet> packet) {
	if(!(*distribution)(generator_loss)) {
		output_port_rl.send(packet);
	}
}
