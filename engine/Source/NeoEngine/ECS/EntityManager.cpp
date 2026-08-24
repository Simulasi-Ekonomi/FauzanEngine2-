#include "EntityManager.h"

EntityID EntityManager::CreateEntity() {
    if (!FreeList.empty()) {
        EntityID id = FreeList.back();
        FreeList.pop_back();
        return id;
    }
    return NextID++;
}

void EntityManager::DestroyEntity(EntityID id) {
    FreeList.push_back(id);
}
