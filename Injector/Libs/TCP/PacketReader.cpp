#include "PacketReader.h"

PacketReader::PacketReader(const char* data, size_t size) : buffer_(data), size_(size), offset_(0) {}

PacketReader::PacketReader(const std::string& data) : PacketReader(data.data(), data.size()) {}

int32_t PacketReader::ReadInt() {
    if (offset_ + 4 > size_) throw std::out_of_range("ReadInt: buffer overrun");
    int32_t val;
    std::memcpy(&val, buffer_ + offset_, 4);
    offset_ += 4;
    return val;
}

uint16_t PacketReader::ReadUShort() {
    if (offset_ + 2 > size_) throw std::out_of_range("ReadUShort: buffer overrun");
    uint16_t val;
    std::memcpy(&val, buffer_ + offset_, 2);
    offset_ += 2;
    return val;
}

uint8_t PacketReader::ReadByte() {
    if (offset_ + 1 > size_) throw std::out_of_range("ReadByte: buffer overrun");
    return static_cast<uint8_t>(buffer_[offset_++]);
}

std::string PacketReader::ReadString(size_t length) {
    if (offset_ + length > size_) throw std::out_of_range("ReadString: buffer overrun");
    std::string s(buffer_ + offset_, length);
    offset_ += length;
    return s;
}

std::string PacketReader::ReadCString() {
    size_t start = offset_;
    while (offset_ < size_ && buffer_[offset_] != '\0') {
        ++offset_;
    }
    if (offset_ >= size_) throw std::out_of_range("ReadCString: unterminated string");
    std::string s(buffer_ + start, offset_ - start);
    ++offset_; // skip null terminator
    return s;
}

size_t PacketReader::GetOffset() const {
    return offset_;
}
