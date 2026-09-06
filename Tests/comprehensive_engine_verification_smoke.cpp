#include "../Source/NeoEngine/Platform/Android/AndroidPlatform.h"
#include "../Source/NeoEngine/Core/Math/Transform.h"
#include "../Source/NeoEngine/Core/Memory/Memory/MemoryManager.h"
#include "../Source/NeoEngine/Core/Memory/MemoryPool.h"
#include "../Source/NeoEngine/AI/V4/AIAgent.h"
#include "../Source/ProjectName/SaveGameBase.h"
#include <iostream>
#include <cmath>

int main() {
    std::cout << "====================================================" << std::endl;
    std::cout << "[FULL ENGINE SMOKE TEST] Running Verification Suite" << std::endl;
    std::cout << "====================================================" << std::endl;

    // 1. Android Platform 100% Capabilities
    std::cout << "[1/5] Testing Android Platform Hardware & Input Queue..." << std::endl;
    auto& platform = NeoEngine::AndroidPlatform::Get();
    platform.Init();
    platform.UpdateHardwareState(95.0f, true, 36.5f);
    platform.SetDisplayMetrics(2560, 1440, 560.0f);

    if (platform.GetBatteryLevel() < 90.0f || platform.GetScreenWidth() != 2560) {
        std::cerr << "FAIL: Android platform hardware state error!" << std::endl;
        return 1;
    }

    NeoEngine::AndroidInputMotionEvent mEv{1, 100.0f, 200.0f, 0, 999};
    platform.InjectMotionEvent(mEv);
    NeoEngine::AndroidInputMotionEvent polledMEv;
    if (!platform.PollMotionEvent(polledMEv) || polledMEv.x != 100.0f) {
        std::cerr << "FAIL: Android motion event polling error!" << std::endl;
        return 1;
    }
    platform.Shutdown();

    // 2. Math Transform Matrix & Quaternions
    std::cout << "[2/5] Testing 3D Math & Transform Matrices..." << std::endl;
    NeoEngine::Transform t;
    t.position = {5.0f, -10.0f, 15.0f};
    t.scale = {3.0f, 3.0f, 3.0f};
    t.rotation = NeoEngine::Quaternion::FromEuler(0.0f, 0.0f, 0.0f);
    NeoEngine::Matrix4 m = t.matrix();
    if (std::abs(m.m[12] - 5.0f) > 0.001f || std::abs(m.m[0] - 3.0f) > 0.001f) {
        std::cerr << "FAIL: 3D Transform matrix calculation error!" << std::endl;
        return 1;
    }

    // 3. Memory Tracking & Allocation Pools
    std::cout << "[3/5] Testing Memory Manager & Memory Pools..." << std::endl;
    MemoryManager::Init();
    void* ptr = MemoryManager::Allocate(1024);
    if (!ptr || MemoryManager::GetTotalAllocatedBytes() != 1024) {
        std::cerr << "FAIL: Memory allocation tracking error!" << std::endl;
        return 1;
    }
    MemoryManager::Free(ptr);
    if (MemoryManager::HasLeaks()) {
        std::cerr << "FAIL: Memory leak false positive!" << std::endl;
        return 1;
    }
    MemoryManager::Shutdown();

    // 4. AI MoE Reasoning Pipeline
    std::cout << "[4/5] Testing AI Mixture-of-Experts Recurrent Agent..." << std::endl;
    NeoEngine::FauzanAIAgent agent;
    NeoEngine::MoEConfig config;
    agent.ConfigureMoE(config);
    std::vector<float> obs(64, 2.0f);
    auto decision = agent.Think(obs, 5);
    if (!decision.success || decision.confidence < 0.8f) {
        std::cerr << "FAIL: AI Reasoning pipeline error!" << std::endl;
        return 1;
    }

    // 5. SaveGame Binary Serialization
    std::cout << "[5/5] Testing SaveGame Binary Serialization..." << std::endl;
    USaveGameBase saveGame;
    saveGame.UserIndex = 0;
    saveGame.PlayerLevel = 99;
    saveGame.PlayTimeSeconds = 7200.0f;
    saveGame.SaveToFile("/tmp/full_savegame_test.bin");

    USaveGameBase loaded;
    if (!loaded.LoadFromFile("/tmp/full_savegame_test.bin") || loaded.PlayerLevel != 99) {
        std::cerr << "FAIL: SaveGame serialization error!" << std::endl;
        return 1;
    }

    std::cout << "====================================================" << std::endl;
    std::cout << "ALL COMPREHENSIVE ENGINE SMOKE TESTS PASSED (100%)!" << std::endl;
    std::cout << "====================================================" << std::endl;
    return 0;
}
