#pragma once
#include "../BasePacket.h"

class ClientboundObfuscationPacket : public BasePacket {
public:
	ClientboundObfuscationPacket(std::string& buffer) : BasePacket(buffer) {
		PacketReader bufferReader(this->_data);

		this->key = bufferReader.ReadCString();
	}

	std::string getKey() {
		return key;
	}

private:
	std::string key;
};