#include "../Source/NeoEngine/Systems/TrustSafetySystem.h"
#include <iostream>

int main() {
    std::cout << "[SMOKE TEST] R10 Security & Privacy Boundary Test..." << std::endl;
    NeoEngine::TrustSafetySystem trust;
    if (trust.Report("Invalid Player!@#$", "evt_001", NeoEngine::FraudSignal::DuplicateReceipt)) {
        std::cerr << "FAIL: Security boundary accepted invalid player ID format!" << std::endl;
        return 1;
    }
    if (!trust.Report("ValidPlayer", "evt_002", NeoEngine::FraudSignal::DuplicateReceipt)) {
        std::cerr << "FAIL: Security boundary rejected valid player ID format!" << std::endl;
        return 1;
    }
    std::cout << "R10 Security & Privacy Smoke Test Passed" << std::endl;
    return 0;
}
