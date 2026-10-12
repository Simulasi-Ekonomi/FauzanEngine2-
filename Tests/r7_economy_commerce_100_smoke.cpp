#include "../Source/NeoEngine/Systems/CommodityEconomyLedger.h"
#include "../Source/NeoEngine/Systems/CommodityCatalog.h"
#include <iostream>
#include <vector>

int main() {
    std::cout << "[SMOKE TEST] R7 Economy & Commerce..." << std::endl;
    NeoEngine::CommodityCatalog catalog;
    NeoEngine::CommodityDefinition def;
    def.id = "C001";
    def.category = NeoEngine::CommodityCategory::Crop;
    def.baseUnitPrice = 50;
    def.maxStack = 999;
    def.tradable = true;
    if (!catalog.Add(def) || catalog.Count() != 1U) return 1;
    if (catalog.Add(def) ||
        catalog.LastError() != NeoEngine::CommodityCatalogError::DuplicateId ||
        catalog.Count() != 1U) return 1;

    NeoEngine::CommodityDefinition invalidDefinition = def;
    invalidDefinition.id = "C002";
    invalidDefinition.category = static_cast<NeoEngine::CommodityCategory>(255U);
    if (catalog.Add(invalidDefinition) ||
        catalog.LastError() != NeoEngine::CommodityCatalogError::InvalidDefinition ||
        catalog.Count() != 1U) return 1;

    NeoEngine::CommodityEconomyLedger ledger;
    NeoEngine::CommodityEconomyCommand cmd;
    cmd.id = "cmd_001";
    cmd.kind = NeoEngine::CommodityEconomyKind::HarvestGrant;
    cmd.commodityId = "C001";
    cmd.quantity = 10;

    if (!ledger.Apply(catalog, cmd) || ledger.Quantity("C001") != 10 || !ledger.HasApplied("cmd_001")) return 1;
    if (ledger.Apply(catalog, cmd) || ledger.LastError() != NeoEngine::CommodityEconomyError::DuplicateCommand || ledger.Quantity("C001") != 10) return 1;

    NeoEngine::CommodityEconomyCommand invalid = cmd;
    invalid.id = "cmd_invalid_kind";
    invalid.kind = static_cast<NeoEngine::CommodityEconomyKind>(255U);
    if (ledger.Apply(catalog, invalid) || ledger.LastError() != NeoEngine::CommodityEconomyError::InvalidCommand || ledger.Quantity("C001") != 10) return 1;

    std::vector<uint8_t> bytes;
    if (!ledger.Serialize(bytes) || bytes.empty()) return 1;
    std::cout << "R7 Economy & Commerce Smoke Test Passed" << std::endl;
    return 0;
}
