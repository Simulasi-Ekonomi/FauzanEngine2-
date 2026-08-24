#pragma once
#include <string>
#include <memory>
#include "Core/Physics/PhysicsCore.h"
#include "World/NeoWorld.h"
#include "AI/AIManager.h"

namespace NeoEngine {

class GameInstanceCore {
public:
    static GameInstanceCore& Get() {
        static GameInstanceCore instance;
        return instance;
    }

    void Initialize(const std::string& appName = "FauzanEngine Game") {
        m_AppName = appName;
        m_World = std::make_unique<NeoWorld>();
        m_Initialized = true;
    }

    void Shutdown() {
        m_World.reset();
        m_Initialized = false;
    }

    NeoWorld* GetWorld() { return m_World.get(); }
    bool IsInitialized() const { return m_Initialized; }

private:
    GameInstanceCore() = default;
    std::string m_AppName;
    std::unique_ptr<NeoWorld> m_World;
    bool m_Initialized = false;
};

} // namespace NeoEngine
