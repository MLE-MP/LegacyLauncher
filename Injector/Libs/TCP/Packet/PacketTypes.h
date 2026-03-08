#pragma once
#include <cstdint>

#include <unordered_map>
#include <functional>

enum PacketTypes : int16_t {
	RegisterAccount = 1,

	AccountError = 55,
};