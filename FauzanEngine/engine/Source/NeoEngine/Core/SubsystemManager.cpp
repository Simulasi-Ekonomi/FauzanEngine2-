#include "SubsystemManager.h"
#include "Subsystem.h"
#include <vector>

static std::vector<Subsystem*> Systems;

void SubsystemManager::Register(Subsystem* system) {
    if (system) {
        Systems.push_back(system);
    }
}

void SubsystemManager::InitAll() {
    for (auto s : Systems) {
        if (s) s->Init();
    }
}

void SubsystemManager::TickAll() {
    for (auto s : Systems) {
        if (s) s->Tick();
    }
}

void SubsystemManager::ShutdownAll() {
    for (auto s : Systems) {
        if (s) s->Shutdown();
    }
    Systems.clear();
}
