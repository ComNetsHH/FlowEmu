#include "Packet.hpp"

Packet::Packet(std::vector<uint8_t> &bytes) : bytes(bytes) {
}

Packet::Packet(uint8_t *data, size_t size) : bytes(data, data + size) {
}

void Packet::setBytes(std::vector<uint8_t> &bytes) {
	this->bytes = bytes;
}

std::vector<uint8_t>& Packet::getBytes() {
	return bytes;
}
