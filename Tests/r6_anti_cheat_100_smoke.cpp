#include "../Source/NeoEngine/Systems/AntiCheatSystem.h"
#include "../Source/NeoEngine/Systems/FraudDetectionSystem.h"
#include "../Source/NeoEngine/Systems/TrustSafetySystem.h"
#include <iostream>

int main() {
    std::cout << "[SMOKE TEST] R6 Anti-Cheat & Fraud Detection 100%..." << std::endl;

    // 1. AntiCheatSystem SpeedHack Detection & Ban
    NeoEngine::AntiCheatSystem antiCheat;
    antiCheat.DetectAndPunish("Player_Cheat1", "Cheater", "speed_hack", "Velocity > 1000", "");
    if (!antiCheat.IsPlayerBanned("Player_Cheat1")) {
        std::cerr << "FAIL: AntiCheatSystem permanent ban not applied!" << std::endl;
        return 1;
    }

    // 2. FraudDetectionSystem & Rapid Trading
    NeoEngine::FraudDetectionSystem fraud;
    for (int i = 0; i < 6; ++i) {
        fraud.ValidateTransaction("BotBuyer", "Seller1", "Item_123", 100);
    }
    bool txOk = fraud.ValidateTransaction("BotBuyer", "Seller1", "Item_123", 100);
    if (txOk) {
        std::cerr << "FAIL: FraudDetectionSystem failed to flag rapid trading!" << std::endl;
        return 1;
    }

    // 3. TrustSafetySystem
    NeoEngine::TrustSafetySystem trust;
    trust.Report("Scammer1", "evt_001", NeoEngine::FraudSignal::DuplicateReceipt);
    trust.Report("Scammer1", "evt_002", NeoEngine::FraudSignal::LedgerMismatch);
    trust.Report("Scammer1", "evt_003", NeoEngine::FraudSignal::ImpossibleInventory);
    if (!trust.IsBanned("Scammer1")) {
        std::cerr << "FAIL: TrustSafetySystem ban accumulation failed!" << std::endl;
        return 1;
    }

    std::cout << "SUCCESS: R6 Anti-Cheat & Fraud Smoke Test Passed (100%)!" << std::endl;
    return 0;
}
