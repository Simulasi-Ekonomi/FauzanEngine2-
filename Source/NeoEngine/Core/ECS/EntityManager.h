#pragma once

#include "Components.h"
#include <cstdint>
#include <vector>

namespace NeoEngine {

class EntityManager {
public:

    static EntityManager& Get()
    {
        static EntityManager instance;
        return instance;
    }

    EntityID CreateEntity();

    void DestroyEntity(EntityID id);

    void DestroyEntityUnsafe(EntityID id);

    bool IsAlive(EntityID id) const
    {
        return static_cast<std::size_t>(id) < alive_.size() && alive_[id] != 0;
    }

    float GetPosX(EntityID id) const
    {
        return IsAlive(id) ? posX_[id] : 0.0f;
    }

    float GetPosZ(EntityID id) const
    {
        return IsAlive(id) ? posZ_[id] : 0.0f;
    }

    float GetVelX(EntityID id) const
    {
        return IsAlive(id) ? velX_[id] : 0.0f;
    }

    float GetVelZ(EntityID id) const
    {
        return IsAlive(id) ? velZ_[id] : 0.0f;
    }

    float GetRadius(EntityID id) const
    {
        return IsAlive(id) ? radius_[id] : 0.0f;
    }

    float GetInvMass(EntityID id) const
    {
        return IsAlive(id) ? invMass_[id] : 0.0f;
    }

private:

    EntityManager() = default;

    // EntityID is a stable slot identity. Destroying an entity never moves
    // another entity into its slot, so existing IDs remain valid.
    std::vector<EntityID> entities_;
    std::vector<std::uint8_t> alive_;
    std::vector<EntityID> freeIds_;

    std::vector<float> posX_;
    std::vector<float> posZ_;

    std::vector<float> velX_;
    std::vector<float> velZ_;

    std::vector<float> radius_;
    std::vector<float> invMass_;
};

}
