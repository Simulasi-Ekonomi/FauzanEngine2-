#include "../Source/NeoEngine/Systems/AntiCheatSystem.h"
#include "../Source/NeoEngine/Systems/FraudDetectionSystem.h"
#include "../Source/NeoEngine/Systems/TrustSafetySystem.h"
#include <chrono>
#include <iostream>

int main() {
    std::cout << "[SMOKE TEST] R6 Anti-Cheat & Fraud Detection..." << std::endl;

    // Exercise the real punishment path without invoking DetectAndPunish(),
    // whose production contract reports synchronously to the remote service.
    // The smoke test must remain hermetic and deterministic/offline.
    NeoEngine::AntiCheatSystem antiCheat;
    NeoEngine::CheatRecord record{
        "Player_Cheat1",
        "Cheater",
        "speed_hack",
        "",
        "Velocity > 1000",
        std::chrono::system_clock::now(),
        NeoEngine::PunishmentLevel::PermaBan,
        false};
    antiCheat.ExecutePunishment(record);
    if (!record.executed || !antiCheat.IsPlayerBanned("Player_Cheat1")) return 1;

    NeoEngine::FraudDetectionSystem fraud;
    for (int i = 0; i < 6; ++i) {
        fraud.ValidateTransaction("BotBuyer", "Seller1", "Item_123", 100);
    }
    if (fraud.ValidateTransaction("BotBuyer", "Seller1", "Item_123", 100)) return 1;

    NeoEngine::TrustSafetySystem trust;
    trust.Report("Scammer1", "evt_001", NeoEngine::FraudSignal::DuplicateReceipt);
    trust.Report("Scammer1", "evt_002", NeoEngine::FraudSignal::LedgerMismatch);
    trust.Report("Scammer1", "evt_003", NeoEngine::FraudSignal::ImpossibleInventory);
    if (!trust.IsBanned("Scammer1")) return 1;

    std::cout << "R6 Anti-Cheat & Fraud Smoke Test Passed" << std::endl;
    return 0;
}
