#pragma once

#include "CommodityCatalog.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace NeoEngine {
enum class CommodityEconomyKind : uint8_t { HarvestGrant, CraftGrant, QuestGrant, ShopBuy, ShopSell };
enum class CommodityEconomyError : uint8_t { None, InvalidCommand, UnknownCommodity, InvalidQuantity, InventoryLimit, InsufficientInventory, InsufficientCoins, Capacity, Overflow, Corrupt, UnsupportedVersion, ChecksumMismatch, TrailingBytes };
struct CommodityEconomyCommand { std::string id; CommodityEconomyKind kind = CommodityEconomyKind::HarvestGrant; std::string commodityId; uint16_t quantity = 0; };
class CommodityEconomyLedger {
public:
    static constexpr uint8_t kVersion = 1;
    static constexpr uint16_t kMaxInventoryEntries = 128;
    static constexpr uint16_t kMaxAppliedCommands = 4096;
    bool Apply(const CommodityCatalog& catalog, const CommodityEconomyCommand& command);
    bool Serialize(std::vector<uint8_t>& out) const;
    bool Deserialize(const CommodityCatalog& catalog, const std::vector<uint8_t>& bytes);
    [[nodiscard]] int64_t Coins() const { return coins_; }
    [[nodiscard]] uint16_t Quantity(std::string_view commodityId) const;
    [[nodiscard]] bool HasApplied(std::string_view commandId) const;
    [[nodiscard]] CommodityEconomyError LastError() const { return lastError_; }
private:
    struct InventoryEntry { std::string commodityId; uint16_t quantity = 0; };
    std::vector<InventoryEntry> inventory_;
    std::vector<std::string> appliedCommandIds_;
    int64_t coins_ = 0;
    CommodityEconomyError lastError_ = CommodityEconomyError::None;
};
} // namespace NeoEngine
