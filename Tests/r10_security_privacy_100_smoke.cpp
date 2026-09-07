#include "../Source/NeoEngine/Core/Memory/Memory/MemoryManager.h"
#include "../Source/NeoEngine/Systems/TrustSafetySystem.h"
#include <iostream>
#include <string>

int main() {
    std::cout << "[SMOKE TEST] R10 Security & Privacy Boundary Test..." << std::endl;

    NeoEngine::TrustSafetySystem trust;
    const bool invalidReport = trust.Report("Invalid Player!@#$", "evt_001", NeoEngine::FraudSignal::DuplicateReceipt);
    if (invalidReport) {
        std::cerr << "FAIL: Security boundary accepted invalid player ID format!" << std::endl;
        return 1;
    }

    MemoryManager::Init();
    void* p = MemoryManager::Allocate(256);
    if (!p) {
        std::cerr << "FAIL: Memory allocation failed!" << std::endl;
        return 1;
    }
    MemoryManager::Free(p);
    if (MemoryManager::HasLeaks()) {
        std::cerr << "FAIL: Memory manager reports an unexpected leak!" << std::endl;
        return 1;
    }
    MemoryManager::Shutdown();

    std::cout << "R10 Security & Privacy Smoke Test Passed" << std::endl;
    return 0;
}
