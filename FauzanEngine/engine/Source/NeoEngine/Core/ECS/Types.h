#pragma once
#include <cstdint>
#include <bitset>

constexpr uint32_t MAX_ENTITIES   = 50000;
constexpr uint32_t MAX_COMPONENTS = 64;

using Signature = std::bitset<MAX_COMPONENTS>;

