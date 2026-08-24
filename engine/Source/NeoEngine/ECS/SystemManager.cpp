#include "SystemManager.h"

void SystemManager::Register(System* system) {
    Systems.push_back(system);
}

void SystemManager::UpdateAll(float deltaTime) {
    for (auto s : Systems) {
        s->Update(deltaTime);
    }
}
