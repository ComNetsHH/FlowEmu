#include "NullModule.hpp"

using namespace std;

NullModule::NullModule() {

}

void NullModule::receiveFromLeftModule(boost::asio::const_buffer packet) {
	passToRightModule(packet);
}

void NullModule::receiveFromRightModule(boost::asio::const_buffer packet) {
	passToLeftModule(packet);
}
