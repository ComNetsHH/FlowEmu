#ifndef NULL_MODULE_HPP
#define NULL_MODULE_HPP

#include <vector>
#include <cstdint>

#include <boost/asio.hpp>

#include "../Module.hpp"

class NullModule : public ModuleHasLeft<boost::asio::const_buffer>, public ModuleHasRight<boost::asio::const_buffer> {
	public:
		NullModule();

		void receiveFromLeftModule(boost::asio::const_buffer packet) override;
		void receiveFromRightModule(boost::asio::const_buffer packet) override;
};

#endif
