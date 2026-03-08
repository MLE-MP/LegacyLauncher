#pragma once
#include "../BasePacket.h"

class ServerboundLoginPacket : public BasePacket {
public:
	ServerboundLoginPacket(std::string& key) : BasePacket(PacketTypes::Login, key) {

	}
};