#ifndef NULL_MODULE_HPP
#define NULL_MODULE_HPP

#include <memory>

#include "../Module.hpp"
#include "../../utils/Packet.hpp"

class NullModule : public Module {
	public:
		NullModule();

	const char* getType() const {
		return "null";
	}

	private:
		ReceivingPort<std::shared_ptr<Packet>> input_port;
		SendingPort<std::shared_ptr<Packet>> output_port;

		void receive(std::shared_ptr<Packet> packet);
};

#endif
