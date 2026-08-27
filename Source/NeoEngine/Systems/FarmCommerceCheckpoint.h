#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace NeoEngine {

class FarmCommerceEntitlementLedger;
class FarmWorldTool;

class FarmCommerceCheckpoint {
public:
    static constexpr size_t kMaxBytes = 262144U;

    static bool Save(const FarmWorldTool& world, const FarmCommerceEntitlementLedger& ledger, std::vector<uint8_t>& bytes);
    static bool Load(std::span<const uint8_t> bytes, FarmWorldTool& world, FarmCommerceEntitlementLedger& ledger);
};

} // namespace NeoEngine
