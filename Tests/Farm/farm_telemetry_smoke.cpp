#include "Systems/FarmSystem.h"
#include "Systems/FarmTelemetryAdapter.h"

#include <cstdio>

using namespace NeoEngine;

int main() {
    FarmSystem farm(20, 50);
    farm.SetReceiptVerifier([](const VerifiedTopUpReceipt& receipt) { return receipt.receiptId == 77 && receipt.authorityPayload == "trusted"; });
    uint32_t units = 0;
    bool ok = farm.Till(0, 0) && farm.Plant(0, 0, FarmCrop::Wheat) && farm.Water(0, 0) && farm.Tick(12) && farm.Harvest(0, 0, units) && farm.Sell(555, FarmItem::WheatProduce, 1, 5) && !farm.Sell(555, FarmItem::WheatProduce, 1, 5) && farm.ApplyVerifiedTopUp({77, 25, "trusted"});
    FarmTelemetryAdapter adapter({"farm-runtime-01", "farm-alpha", "neo-0.1", "player-01"});
    std::string json;
    ok = ok && adapter.BuildEnvelope(farm, 1700000000000ULL, json);
    ok = ok && json.find("\"sourceRef\":\"farm-runtime-01\"") != std::string::npos && json.find("\"tileCount\":1000") != std::string::npos && json.find("\"verifiedReceipts\":1") != std::string::npos && json.find("\"rejectedTransactions\":1") != std::string::npos && json.find("farm.topup_accepted") != std::string::npos;
    if (!ok) { std::fprintf(stderr, "FARM_TELEMETRY_SMOKE_FAIL\n"); return 1; }
    std::printf("FARM_TELEMETRY_SMOKE_OK bytes=%zu events=%zu\n", json.size(), farm.RecentEvents().size());
    return 0;
}
