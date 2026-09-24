#include "../../Source/NeoEngine/Systems/ItemSerialTracker.h"
#include <iostream>

int main() {
    NeoEngine::ItemSerialTracker tracker;

    bool cheatCallback = false;
    tracker.SetOnCheatDetected([&](const std::string&, const std::string&) { cheatCallback = true; });

    auto* item = tracker.RegisterItem("PlayerA", "material", "Iron", 3, "harvest");
    if (item == nullptr || item->number.empty() || item->quantity != 3) return 1;
    const std::string serial = item->number;

    if (tracker.IsItemValid(serial)) return 1;
    if (tracker.VerifyPendingBatch(1) != 1) return 1;
    if (!tracker.IsItemValid(serial)) return 1;
    if (!tracker.VerifyItemSilently(serial, "PlayerA")) return 1;

    if (!tracker.TransferOwnership(serial, "PlayerA", "Alice", "PlayerB", "Bob", "trade")) return 1;
    if (tracker.VerifyItemSilently(serial, "PlayerA")) return 1;
    if (!tracker.VerifyItemSilently(serial, "PlayerB")) return 1;

    tracker.MarkAllPlayerItemsContaminated("PlayerB", "Bob");
    if (!cheatCallback || tracker.IsItemValid(serial)) return 1;

    const std::string rewardSerial = tracker.GenerateRewardSerial("GoldenSeed", "quest");
    if (rewardSerial.empty() || tracker.VerifyPendingBatch(1) != 1) return 1;
    if (!tracker.IsItemValid(rewardSerial)) return 1;
    if (!tracker.ConsumeItem(rewardSerial) || !tracker.IsSerialDuplicate(rewardSerial)) return 1;
    if (tracker.ConsumeItem(rewardSerial)) return 1;

    const std::string audit = tracker.GetAuditTrail("PlayerB");
    if (audit.find("TRANSFER|" + serial + "|PlayerA|PlayerB|trade") == std::string::npos) return 1;
    if (audit.find("CONTAMINATE|" + serial + "|PlayerB") == std::string::npos) return 1;

    std::cout << "ItemSerialTracker Smoke Test Passed" << std::endl;
    return 0;
}
