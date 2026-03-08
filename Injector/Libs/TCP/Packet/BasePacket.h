#pragma once
#include <cstdint>
#include "../PacketReader.h"
#include "PacketTypes.h"
#include <string>

#include "../PacketWriter.h"

class BasePacket {
public:

	BasePacket(const std::string& buffer) {
		PacketReader bufferReader(buffer);

		_dataLength = bufferReader.ReadPrimitive<int32_t>();
		_packetType = bufferReader.ReadPrimitive<uint8_t>();
		 //_isObfuscated = (bufferReader.ReadPrimitive<char>() == 1);// strict conversion cause yes

		 //if (_isObfuscated && savedKey.empty()) return;

		 std::string straightData = buffer.substr(11);

		 //if (_isObfuscated) {
		 //	 this->_data = RemoveObfuscation(straightData, savedKey);
		 //} else {
		 //	  this->_data = straightData;
		 //}
	}

	BasePacket(PacketTypes packetType, std::string& data) : _dataLength(data.size()), _packetType(packetType),  /*_isObfuscated(false),*/ _data(data) {}
	BasePacket(PacketTypes packetType) : _dataLength(-1), _packetType(packetType),  /*_isObfuscated(false),*/ _data("") {}
	/*BasePacket(PacketTypes packetType, std::string& data, std::string& key) {
		std::string obfuscatedData = AddObfuscation(data, key);

		_dataLength = obfuscatedData.size();
		_packetType = packetType;
		_isObfuscated = true;
		_data = obfuscatedData;
	}*/

	void CalculateData(PacketWriter writer) {
		_data = writer.GetBuffer();
		_dataLength = _data.length();
	}

	PacketTypes getPacketType() const {
		return (PacketTypes)(_packetType);
	}

	std::string getString() {
		std::string finalData(5, 255);

		finalData += _data;

		finalData[0] = _dataLength & 0xFF;
		finalData[1] = (_dataLength >> 8) & 0xFF;
		finalData[2] = (_dataLength >> 16) & 0xFF;
		finalData[3] = (_dataLength >> 24) & 0xFF;

		finalData[4] = (char)_packetType;

		//finalData[8] = (_isObfuscated ? 1 : 0);//strict converstion

		return finalData;
	}

	/*static void readyObfuscation(std::string key) {
		savedKey = key;
	}

	static bool isObfuscationReady() {
		return !savedKey.empty();
	}*/

private:
	int32_t _dataLength;
	uint8_t _packetType;
	//bool  _isObfuscated;

	//static std::string savedKey;
	

	/*static std::string AddObfuscation(const std::string& input, const std::string& key) {
		std::string output = input;
		size_t keyLen = key.length();

		for (size_t i = 0; i < input.size(); ++i) {
			output[i] = static_cast<char>((static_cast<uint8_t>(input[i]) + static_cast<uint8_t>(key[i % keyLen]) + i) % 256);
		}

		return output;
	}

	static std::string RemoveObfuscation(const std::string& input, const std::string& key) {
		std::string output = input;
		size_t keyLen = key.length();

		for (size_t i = 0; i < input.size(); ++i) {
			output[i] = static_cast<char>((static_cast<int>(static_cast<uint8_t>(input[i])) - static_cast<uint8_t>(key[i % keyLen]) - static_cast<int>(i) + 256) % 256);
		}

		return output;
	}*/

protected:
	std::string _data;
};