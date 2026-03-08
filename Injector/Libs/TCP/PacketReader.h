#pragma once
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <cstring>

class PacketReader {
public:
    PacketReader(const char* data, size_t size);
    PacketReader(const std::string& data);

    int32_t ReadInt();       // 4 bytes
    uint16_t ReadUShort();   // 2 bytes
    uint8_t ReadByte();      // 1 byte
    std::string ReadString(size_t length); // fixed-length string
    std::string ReadCString();             // null-terminated string


    template<typename T>
    T ReadPrimitive();

    size_t GetOffset() const;

private:
    const char* buffer_;
    size_t size_;
    size_t offset_;
    
};

template <typename T>
T PacketReader::ReadPrimitive() {
    if (offset_ + sizeof(T) > size_) throw std::out_of_range("ReadPrimitive: buffer overrun");
    T val;
    std::memcpy(&val, buffer_ + offset_, sizeof(T));
    offset_ += sizeof(T);
    return val;
}
