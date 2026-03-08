#pragma once
#include "../BasePacket.h"

#include "../../PacketWriter.h"

class ServerboundRegisterAccountPacket : public BasePacket {
public:
	ServerboundRegisterAccountPacket(std::string& username, std::string& password) : BasePacket(PacketTypes::RegisterAccount) {
		PacketWriter Writer = PacketWriter();
		Writer.WriteString(username);
		Writer.WriteString(password);

		CalculateData(Writer);
	}
};