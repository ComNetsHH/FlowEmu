#ifndef PACKET_HPP
#define PACKET_HPP

#include <vector>
#include <cstdint>

class Packet {
	public:
		Packet(std::vector<uint8_t> &bytes);
		Packet(uint8_t *data, size_t size);

		void setBytes(std::vector<uint8_t> &bytes);
		std::vector<uint8_t>& getBytes();

	private:
		std::vector<uint8_t> bytes;
};

#endif
