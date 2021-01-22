#include "NullModule.hpp"

using namespace std;

NullModule::NullModule() {

}

void NullModule::receiveFromLeftModule(shared_ptr<Packet> packet) {
	passToRightModule(packet);
}

void NullModule::receiveFromRightModule(shared_ptr<Packet> packet) {
	passToLeftModule(packet);
}
