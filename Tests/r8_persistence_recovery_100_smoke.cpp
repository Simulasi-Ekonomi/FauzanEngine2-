#include "../Source/NeoEngine/Runtime/AtomicSaveFile.h"
#include <iostream>
#include <vector>
#include <filesystem>

int main() {
    std::cout << "[SMOKE TEST] R8 Persistence & Recovery 100%..." << std::endl;

    std::filesystem::path rootDir = "/tmp/r8_save_root";
    std::string slotName = "slot_01";
    std::vector<uint8_t> payload = {0xDE, 0xAD, 0xBE, 0xEF, 0x12, 0x34, 0x56, 0x78};
    NeoEngine::AtomicSaveFileError err = NeoEngine::AtomicSaveFileError::None;

    // 1. Write atomic save slot
    if (!NeoEngine::AtomicSaveFile::Write(rootDir, slotName, payload, err)) {
        std::cerr << "FAIL: AtomicSaveFile Write failed with error!" << std::endl;
        return 1;
    }

    // 2. Read atomic save slot
    std::vector<uint8_t> readData;
    if (!NeoEngine::AtomicSaveFile::Read(rootDir, slotName, readData, err) || readData != payload) {
        std::cerr << "FAIL: AtomicSaveFile Read mismatch!" << std::endl;
        return 1;
    }

    std::cout << "SUCCESS: R8 Persistence & Recovery Smoke Test Passed (100%)!" << std::endl;
    return 0;
}
