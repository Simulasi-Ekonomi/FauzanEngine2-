#pragma once
#include "EntityManager.h"
#include "SystemManager.h"

class World {
public:
    EntityManager Entities;
    SystemManager Systems;
    void Update(float deltaTime);
};
