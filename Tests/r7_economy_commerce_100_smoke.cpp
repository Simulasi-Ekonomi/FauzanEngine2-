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
    catalog.Add(def);

    NeoEngine::CommodityEconomyLedger ledger;
    NeoEngine::CommodityEconomyCommand cmd;
    cmd.id = "cmd_001";
    cmd.kind = NeoEngine::CommodityEconomyKind::HarvestGrant;
    cmd.commodityId = "C001";
    cmd.quantity = 10;

    if (!ledger.Apply(catalog, cmd) || ledger.Quantity("C001") != 10 || !ledger.HasApplied("cmd_001")) return 1;
    if (!ledger.Apply(catalog, cmd) || ledger.Quantity("C001") != 10 || !ledger.HasApplied("cmd_001")) return 1;

    std::vector<uint8_t> bytes;
    if (!ledger.Serialize(bytes) || bytes.empty()) return 1;
    std::cout << "R7 Economy & Commerce Smoke Test Passed" << std::endl;
    return 0;
}
