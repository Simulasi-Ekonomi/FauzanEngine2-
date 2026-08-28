#include "Systems/FarmSystem.h"
#include "Systems/FarmTelemetryAdapter.h"

#include <cstdio>

using namespace NeoEngine;

int main() {
    FarmSystem farm(20, 50);
    farm.SetReceiptVerifier([](const VerifiedTopUpReceipt& receipt) { return receipt.receiptId == 77 && receipt.authorityPayload == "trusted"; });
    uint32_t units = 0;
    bool ok = farm.Till(0, 0) && farm.Plant(0, 0, FarmCrop::Wheat) && farm.Water(0, 0) && farm.Tick(12) &&
              farm.Harvest(0, 0, units) && farm.Sell(555, FarmItem::WheatProduce, 1, 5) &&
              !farm.Sell(555, FarmItem::WheatProduce, 1, 5) && farm.ApplyVerifiedTopUp({77, 25, "trusted"});
    FarmTelemetryAdapter privacyAdapter({"farm-runtime-01", "farm-alpha", "neo-0.1", "player-01"});
    std::string json;
    ok = ok && privacyAdapter.BuildEnvelope(farm, 1700000000000ULL, json);
    ok = ok && json.find("\"schemaVersion\":1") != std::string::npos &&
         json.find("\"sourceRef\":\"farm-runtime-01\"") != std::string::npos &&
         json.find("\"tileCount\":1000") != std::string::npos &&
         json.find("\"verifiedReceipts\":1") != std::string::npos &&
         json.find("\"rejectedTransactions\":1") != std::string::npos &&
         json.find("farm.topup_accepted") != std::string::npos &&
         json.find("\"playerId\"") == std::string::npos && json.find("\"goldBalance\"") == std::string::npos &&
         json.find("\"detail\"") == std::string::npos;

    FarmTelemetryAdapter diagnosticAdapter({"farm-runtime-01", "farm-alpha", "neo-0.1", "player-01"},
                                           FarmTelemetryPolicy{true, true, true, 64U});
    std::string diagnosticJson;
    ok = ok && diagnosticAdapter.BuildEnvelope(farm, 1700000000000ULL, diagnosticJson) &&
         diagnosticJson.find("\"playerId\":\"player-01\"") != std::string::npos &&
         diagnosticJson.find("\"goldBalance\"") != std::string::npos &&
         diagnosticJson.find("\"detail\":\"value=") != std::string::npos;

    FarmTelemetryPolicy invalidPolicy{false, false, false, 65U};
    FarmTelemetryAdapter invalidAdapter({"farm-runtime-01", "farm-alpha", "neo-0.1", "player-01"}, invalidPolicy);
    std::string rejectedJson = "preserved";
    ok = ok && !invalidAdapter.BuildEnvelope(farm, 1700000000000ULL, rejectedJson) && rejectedJson.empty();
    if (!ok) {
        std::fprintf(stderr, "FARM_TELEMETRY_SMOKE_FAIL\n");
        return 1;
    }
    std::printf("FARM_TELEMETRY_SMOKE_OK bytes=%zu diagnostic_bytes=%zu events=%zu privacy=default-redacted cap=64\n",
                json.size(), diagnosticJson.size(), farm.RecentEvents().size());
    return 0;
}
