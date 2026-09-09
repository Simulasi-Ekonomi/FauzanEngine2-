#include "EntityManager.h"

#include <limits>

namespace NeoEngine {

EntityID EntityManager::CreateEntity() {
    EntityID id;

    if (!freeIds_.empty()) {
        id = freeIds_.back();
        freeIds_.pop_back();
        alive_[id] = 1;
        entities_[id] = id;

        posX_[id] = 0.0f;
        posZ_[id] = 0.0f;
        velX_[id] = 0.0f;
        velZ_[id] = 0.0f;
        radius_[id] = 1.0f;
        invMass_[id] = 1.0f;
        return id;
    }

    if (entities_.size() > static_cast<std::size_t>(std::numeric_limits<EntityID>::max())) {
        return std::numeric_limits<EntityID>::max();
    }

    id = static_cast<EntityID>(entities_.size());
    entities_.push_back(id);
    alive_.push_back(1);
    posX_.push_back(0.0f);
    posZ_.push_back(0.0f);
    velX_.push_back(0.0f);
    velZ_.push_back(0.0f);
    radius_.push_back(1.0f);
    invMass_.push_back(1.0f);
    return id;
}

void EntityManager::DestroyEntity(EntityID id) {
    if (!IsAlive(id)) return;

    // Stable IDs: never move another entity into this slot.
    alive_[id] = 0;
    freeIds_.push_back(id);
}

void EntityManager::DestroyEntityUnsafe(EntityID id) {
    DestroyEntity(id);
}

} // namespace NeoEngine
