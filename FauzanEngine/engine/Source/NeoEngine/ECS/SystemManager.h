#pragma once
#include <vector>
#include "System.h"

class SystemManager {
public:
    void Register(System* system);
    void UpdateAll(float deltaTime);
private:
    std::vector<System*> Systems;
};
