#include "PacketWriter.h"

PacketWriter::PacketWriter() {
    buffer_.clear();
}

void PacketWriter::WriteByte(uint8_t value) {
    buffer_ += static_cast<char>(value);
}

void PacketWriter::WriteShort(int16_t value) {
    buffer_.append(reinterpret_cast<const char*>(&value), sizeof(value));
}

void PacketWriter::WriteUShort(uint16_t value) {
    buffer_.append(reinterpret_cast<const char*>(&value), sizeof(value));
}

void PacketWriter::WriteInt(int32_t value) {
    buffer_.append(reinterpret_cast<const char*>(&value), sizeof(value));
}

void PacketWriter::WriteString(const std::string& value) {
    uint16_t length = static_cast<uint16_t>(value.length());

    WriteUShort(length);

    for (char c : value) {
        uint16_t utf16 = static_cast<uint16_t>(c);
        buffer_.append(reinterpret_cast<const char*>(&utf16), sizeof(uint16_t));
    }
}

const std::string& PacketWriter::GetBuffer() const {
    return buffer_;
}

std::string& PacketWriter::GetBuffer() {
    return buffer_;
}