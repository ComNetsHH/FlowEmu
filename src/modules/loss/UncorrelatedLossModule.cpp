#include "UncorrelatedLossModule.hpp"

using namespace std;

UncorrelatedLossModule::UncorrelatedLossModule(double p, uint32_t seed_loss) : distribution(p), generator_loss(seed_loss) {

}

void UncorrelatedLossModule::receiveFromLeftModule(boost::asio::const_buffer packet) {
	if(!distribution(generator_loss)) {
		passToRightModule(packet);
	}
}

void UncorrelatedLossModule::receiveFromRightModule(boost::asio::const_buffer packet) {
	//if(!distribution(generator_loss)) {
		passToLeftModule(packet);
	//}
}
