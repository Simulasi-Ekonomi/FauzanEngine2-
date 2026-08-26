#pragma once

#include "RuntimePersistence.h"

#include <cstdint>
#include <vector>

namespace NeoEngine {
class FarmSystem;
enum class FarmRuntimeSaveError : uint8_t { None, InvalidRevision, EnvelopeFailed, WrongKind, FarmDeserializeFailed };
class FarmRuntimeSaveCodec {
public:
    static constexpr const char* kKind = "farm-world";
    static bool Encode(const FarmSystem& farm, uint64_t revision, std::vector<uint8_t>& bytes, FarmRuntimeSaveError& error);
    static bool Decode(FarmSystem& farm, const std::vector<uint8_t>& bytes, uint64_t& revision, FarmRuntimeSaveError& error);
};
} // namespace NeoEngine
