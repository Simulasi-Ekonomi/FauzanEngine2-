#pragma once
#include "IComponentStorage.h"
#include "Entity.h"
#include "LockFreeSparseSet.h"

namespace NeoEngine {

// INDUSTRY AAA LOCK-FREE STORAGE
template<typename T>
class LockFreeComponentStorage : public IComponentStorage {
private:
    LockFreeSparseSet<Entity> entities;

public:
    void Add(Entity e) {
        entities.Insert(e);
    }

    bool Has(Entity e) {
        return entities.Contains(e);
    }
};

}
