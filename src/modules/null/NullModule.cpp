#include "NullModule.hpp"

using namespace std;

NullModule::NullModule() {
	setName("Null");
	addPort({"in", "In", PortInfo::Side::left, &input_port});
	addPort({"out", "Out", PortInfo::Side::right, &output_port});

	input_port.setReceiveHandler(bind(&NullModule::receive, this, placeholders::_1));
}

void NullModule::receive(shared_ptr<Packet> packet) {
	output_port.send(packet);
}
