#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace NeoEngine {

class FarmAuthoritativeService;
class FarmWorldTool;

class FarmAuthorityCheckpoint {
public:
    static constexpr size_t kMaxBytes = 8U * 1024U * 1024U;

    static bool Save(const FarmWorldTool& world, const FarmAuthoritativeService& service, std::vector<uint8_t>& bytes);
    static bool Load(std::span<const uint8_t> bytes, FarmWorldTool& world, FarmAuthoritativeService& service);
};

} // namespace NeoEngine
