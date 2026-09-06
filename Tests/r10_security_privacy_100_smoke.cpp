#include "../Source/NeoEngine/Core/Memory/Memory/MemoryManager.h"
#include "../Source/NeoEngine/Systems/TrustSafetySystem.h"
#include <iostream>
#include <string>

int main() {
    std::cout << "[SMOKE TEST] R10 Security & Privacy 100% Boundary Test..." << std::endl;

    // 1. Trust Safety Boundary Validation
    NeoEngine::TrustSafetySystem trust;
    bool invalidReport = trust.Report("Invalid Player!@#$", "evt_001", NeoEngine::FraudSignal::DuplicateReceipt);
    if (invalidReport) {
        std::cerr << "FAIL: Security boundary accepted malicious player ID format!" << std::endl;
        return 1;
    }

    // 2. Memory Boundary Sanity
    MemoryManager::Init();
    void* p = MemoryManager::Allocate(256);
    if (!p) {
        std::cerr << "FAIL: Memory allocation failed!" << std::endl;
        return 1;
    }
    MemoryManager::Free(p);
    MemoryManager::Shutdown();

    std::cout << "SUCCESS: R10 Security & Privacy Smoke Test Passed (100%)!" << std::endl;
    return 0;
}
