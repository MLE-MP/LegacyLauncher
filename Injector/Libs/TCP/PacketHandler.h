#pragma once
#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include "Packet/BasePacket.h"
#include "PacketReader.h"

class PacketHandler {
public:
    using PacketCallback = std::function<void(const BasePacket*)>;

    static void feed(const std::string& newData);
    static void onPacket(PacketCallback cb);

    static std::string debug;
private:
    static std::string buffer_;
    static PacketCallback callback_;

};
