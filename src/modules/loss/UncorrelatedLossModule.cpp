#include "UncorrelatedLossModule.hpp"

using namespace std;

UncorrelatedLossModule::UncorrelatedLossModule(double loss, uint32_t seed) {
	setName("Uncorrelated Loss");
	addPort({"lr_in", "In", PortInfo::Side::left, &input_port_lr});
	addPort({"lr_out", "Out", PortInfo::Side::right, &output_port_lr});
	addPort({"rl_in", "In", PortInfo::Side::right, &input_port_rl});
	addPort({"rl_out", "Out", PortInfo::Side::left, &output_port_rl});
	addParameter({"loss", "Loss", "%", &parameter_loss});
	addParameter({"seed", "Seed", "", &parameter_seed});

	input_port_lr.setReceiveHandler(bind(&UncorrelatedLossModule::receiveFromLeftModule, this, placeholders::_1));
	input_port_rl.setReceiveHandler(bind(&UncorrelatedLossModule::receiveFromRightModule, this, placeholders::_1));

	parameter_loss.addChangeHandler([&](double value) {
		distribution.reset(new bernoulli_distribution(value / 100));
	});
	parameter_seed.addChangeHandler([&](double value) {
		generator_loss.seed(value);
	});

	parameter_loss.set(loss);
	parameter_seed.set(seed);
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
