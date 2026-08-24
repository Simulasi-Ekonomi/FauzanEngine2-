#include "EntityManager.h"

namespace NeoEngine {

EntityID EntityManager::CreateEntity() {
    EntityID id = entities_.size();
    entities_.push_back(id);
    posX_.push_back(0.0f);
    posZ_.push_back(0.0f);
    velX_.push_back(0.0f);
    velZ_.push_back(0.0f);
    radius_.push_back(1.0f);
    invMass_.push_back(1.0f);
    return id;
}

void EntityManager::DestroyEntity(EntityID id) {
    if (id >= entities_.size()) return;
    // swap dengan elemen terakhir
    EntityID last = entities_.back();
    entities_[id] = last;
    posX_[id] = posX_.back();
    posZ_[id] = posZ_.back();
    velX_[id] = velX_.back();
    velZ_[id] = velZ_.back();
    radius_[id] = radius_.back();
    invMass_[id] = invMass_.back();
    entities_.pop_back();
    posX_.pop_back();
    posZ_.pop_back();
    velX_.pop_back();
    velZ_.pop_back();
    radius_.pop_back();
    invMass_.pop_back();
}

void EntityManager::DestroyEntityUnsafe(EntityID id) {
    DestroyEntity(id);
}

} // namespace NeoEngine
