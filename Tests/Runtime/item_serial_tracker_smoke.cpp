#include "../../Source/NeoEngine/Systems/ItemSerialTracker.h"

#include <iostream>
#include <string>

int main() {
    NeoEngine::ItemSerialTracker tracker;
    int cheatSignals = 0;
    tracker.SetOnCheatDetected([&](const std::string&, const std::string&) { ++cheatSignals; });

    NeoEngine::SerialNumber* item =
        tracker.RegisterItem("player_a", "weapon", "TestBlade", 1, "reward");
    if (!item || item->number.empty()) return 1;

    const std::string serial = item->number;
    if (tracker.VerifyPendingBatch(1) != 1) return 2;
    if (!tracker.IsItemValid(serial)) return 3;
    if (!tracker.VerifyOwnershipIntegrity(serial)) return 4;

    if (tracker.TransferOwnership(serial, "wrong_owner", "Wrong", "player_b", "Buyer", "trade"))
        return 5;
    if (cheatSignals != 1) return 6;

    if (!tracker.TransferOwnership(serial, "player_a", "Alice", "player_b", "Bob", "trade"))
        return 7;
    if (!tracker.IsItemValid(serial)) return 8;
    if (!tracker.VerifyOwnershipIntegrity(serial)) return 9;

    const std::string chain = tracker.GetOwnershipChain(serial);
    if (chain.find("player_a") == std::string::npos ||
        chain.find("player_b") == std::string::npos)
        return 10;

    tracker.MarkAllPlayerItemsContaminated("player_a", "Alice");
    if (tracker.IsItemValid(serial)) return 11;
    if (!tracker.IsSerialDuplicate(serial)) return 12;

    if (tracker.ConsumeItem(serial)) return 13;
    if (tracker.GetVerifiedCount() != 1) return 14;

    std::cout << "ITEM_SERIAL_TRACKER_SMOKE_OK verified="
              << tracker.GetVerifiedCount()
              << " cheat_signals=" << cheatSignals << "\n";
    return 0;
}
