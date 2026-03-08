#pragma once
#include <string>
#include <cstdint>

class PacketWriter {
private:
    std::string buffer_;

public:
    PacketWriter();

    void WriteByte(uint8_t value);
    void WriteShort(int16_t value);
    void WriteUShort(uint16_t value);
    void WriteInt(int32_t value);
    void WriteString(const std::string& value);

    const std::string& GetBuffer() const;
    std::string& GetBuffer();
};