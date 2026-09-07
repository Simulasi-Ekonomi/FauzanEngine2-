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
    NeoEngine::CommodityEconomyCommand cmd1;
    cmd1.id = "cmd_001";
    cmd1.kind = NeoEngine::CommodityEconomyKind::HarvestGrant;
    cmd1.commodityId = "C001";
    cmd1.quantity = 10;

    if (!ledger.Apply(catalog, cmd1)) {
        std::cerr << "FAIL: Commodity ledger apply failed!" << std::endl;
        return 1;
    }
    if (ledger.Quantity("C001") != 10) {
        std::cerr << "FAIL: Ledger quantity mismatch!" << std::endl;
        return 1;
    }
    if (!ledger.HasApplied("cmd_001")) {
        std::cerr << "FAIL: HasApplied returned false for applied command!" << std::endl;
        return 1;
    }

    if (ledger.Apply(catalog, cmd1) || ledger.Quantity("C001") != 10) {
        std::cerr << "FAIL: Idempotency check failed!" << std::endl;
        return 1;
    }

    std::vector<uint8_t> bytes;
    if (!ledger.Serialize(bytes) || bytes.empty()) {
        std::cerr << "FAIL: Commodity ledger serialization failed!" << std::endl;
        return 1;
    }

    std::cout << "R7 Economy & Commerce Smoke Test Passed" << std::endl;
    return 0;
}
