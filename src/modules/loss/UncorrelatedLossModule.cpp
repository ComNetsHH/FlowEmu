#include "UncorrelatedLossModule.hpp"

using namespace std;

UncorrelatedLossModule::UncorrelatedLossModule(double p, uint32_t seed_loss) : generator_loss(seed_loss) {
	setLossProbability(p);
}

void UncorrelatedLossModule::setLossProbability(double p) {
	distribution.reset(new bernoulli_distribution(p));
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
