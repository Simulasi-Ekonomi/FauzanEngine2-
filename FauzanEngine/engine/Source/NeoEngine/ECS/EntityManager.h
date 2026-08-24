#pragma once
#include "Entity.h"
#include <vector>

class EntityManager {
public:
    EntityID CreateEntity();
    void DestroyEntity(EntityID id);
private:
    EntityID NextID = 1;
    std::vector<EntityID> FreeList;
};
