#pragma once
#include <vector>

namespace NeoEngine {

class Subsystem;

class SubsystemManager {
public:
    static void Register(Subsystem* system);
    static void InitAll();
    static void TickAll();
    static void ShutdownAll();
    static bool AreAllInitialized();

private:
    static std::vector<Subsystem*> s_Systems;
    static bool s_AllInitialized;
};

} // namespace NeoEngine
