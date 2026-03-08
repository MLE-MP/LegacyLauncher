#pragma once
#include "../BasePacket.h"

class ClientboundAccountErrorPacket : public BasePacket {
public:
	ClientboundAccountErrorPacket(std::string& buffer) : BasePacket(buffer) {
		PacketReader bufferReader(this->_data);

		this->errorCode = bufferReader.ReadByte();
	}

	std::string getError() {
		switch (errorCode) {
		case 1:
			return "This Username Has Been Blacklisted";
		default:
			return ("Unknown Error: " + std::to_string(errorCode));
		}
	}

private:
	uint8_t errorCode;
};