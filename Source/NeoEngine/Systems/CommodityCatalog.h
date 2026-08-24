#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace NeoEngine {
enum class CommodityCategory : uint8_t { Crop, AnimalProduct, Ore, CraftedGood, ShopGood, QuestGood };
enum class CommodityCatalogError : uint8_t { None, InvalidId, DuplicateId, InvalidDefinition, Capacity, Corrupt, UnsupportedVersion, ChecksumMismatch, TrailingBytes };
struct CommodityDefinition { std::string id; CommodityCategory category = CommodityCategory::Crop; int64_t baseUnitPrice = 0; uint16_t maxStack = 0; bool tradable = true; };
class CommodityCatalog {
public:
    static constexpr uint8_t kVersion = 1;
    static constexpr uint8_t kMaxCommodities = 128;
    static constexpr uint8_t kMaxIdBytes = 48;
    bool Add(CommodityDefinition definition);
    [[nodiscard]] const CommodityDefinition* Find(std::string_view id) const;
    bool Serialize(std::vector<uint8_t>& out) const;
    bool Deserialize(const std::vector<uint8_t>& bytes);
    [[nodiscard]] size_t Count() const { return definitions_.size(); }
    [[nodiscard]] CommodityCatalogError LastError() const { return lastError_; }
private:
    std::vector<CommodityDefinition> definitions_;
    CommodityCatalogError lastError_ = CommodityCatalogError::None;
};
} // namespace NeoEngine
