#include "PacketHandler.h"
#include <cstring>

//#include "Packet/Types/ClientboundObfuscationPacket.h"
#include "Packet/Types/ClientboundAccountErrorPacket.h"

#include <iostream> // Add this include for std::cout

#include <Windows.h>
#include <consoleapi.h>

std::string PacketHandler::buffer_ = "";
PacketHandler::PacketCallback PacketHandler::callback_ = NULL;

void PacketHandler::onPacket(PacketCallback cb) {
    callback_ = std::move(cb);
}


std::string PacketHandler::debug = "";

void PacketHandler::feed(const std::string& newData) {
    buffer_ += newData;

    const size_t HEADER_SIZE = 5;

    PacketHandler::debug = newData;

    while (buffer_.size() >= HEADER_SIZE) {
        const char* data = buffer_.data();

        uint32_t packetSize = (uint8_t)data[0] | ((uint8_t)data[1] << 8) | ((uint8_t)data[2] << 16) | ((uint8_t)data[3] << 24);

        size_t totalSize = HEADER_SIZE + packetSize;
        if (buffer_.size() < totalSize)
            return; // wait for more data

        uint8_t packetIndex;
        std::memcpy(&packetIndex, data + 4, 1);

        BasePacket* packetToHandle;

        std::string payload = buffer_.substr(HEADER_SIZE, packetSize);

        switch (packetIndex) {
            /*case PacketTypes::StartObfuscation:
                packetToHandle = new ClientboundObfuscationPacket(buffer_);
                break;
            */
            case PacketTypes::AccountError:
                packetToHandle = new ClientboundAccountErrorPacket(payload);
                break;
            default:
                packetToHandle = new BasePacket(payload);
                break;
        }

        if (callback_)
            callback_(packetToHandle);

        delete packetToHandle;

        buffer_ = buffer_.substr(totalSize); // remove parsed packet
    }
}
