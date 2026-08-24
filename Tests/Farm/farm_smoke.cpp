#include "Systems/FarmSystem.h"
#include <cstdio>

using namespace NeoEngine;

int main() {
    FarmSystem farm(20, 50);
    if (!farm.IsReady()) return 1;
    farm.SetReceiptVerifier([](const VerifiedTopUpReceipt& receipt) { return receipt.receiptId == 42 && receipt.authorityPayload == "authority:verified"; });
    bool ok = true;
    for (uint16_t x = 0; x < 5; ++x) {
        uint32_t units = 0;
        ok = ok && farm.Till(x, 0) && farm.Plant(x, 0, FarmCrop::Wheat) && farm.Water(x, 0);
        ok = ok && farm.Tick(12) && farm.Harvest(x, 0, units) && units == 2;
    }
    ok = ok && farm.ItemCount(FarmItem::WheatProduce) == 10;
    ok = ok && farm.Sell(1001, FarmItem::WheatProduce, 6, 7) && !farm.Sell(1001, FarmItem::WheatProduce, 1, 7);
    ok = ok && !farm.ApplyVerifiedTopUp({7, 100, "untrusted"});
    ok = ok && farm.ApplyVerifiedTopUp({42, 100, "authority:verified"}) && !farm.ApplyVerifiedTopUp({42, 100, "authority:verified"});
    ok = ok && farm.AddAnimal(FarmAnimal::Hen) && farm.Tick(12) && farm.ItemCount(FarmItem::Egg) == 1;
    const auto snapshot = farm.Snapshot();
    const auto bytes = farm.Serialize();
    FarmSystem restored(1, 1);
    ok = ok && restored.Deserialize(bytes) && restored.Coins() == farm.Coins() && restored.ItemCount(FarmItem::Egg) == 1 && restored.Snapshot().questCompleted;
    if (!ok) { std::fprintf(stderr, "FARM_SMOKE_FAIL error=%u\n", static_cast<unsigned>(farm.LastError())); return 1; }
    std::printf("FARM_SMOKE_OK tiles=%u harvestable=%u coins=%lld events=%zu bytes=%zu\n", snapshot.tilledTiles + snapshot.growingTiles + snapshot.harvestableTiles, snapshot.harvestableTiles, static_cast<long long>(farm.Coins()), farm.RecentEvents().size(), bytes.size());
    return 0;
}
