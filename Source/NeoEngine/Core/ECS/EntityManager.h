#pragma once

#include "Components.h"
#include <vector>
#include <cstdint>

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


    float GetPosX(EntityID id) const
    {
        return posX_[id];
    }

    float GetPosZ(EntityID id) const
    {
        return posZ_[id];
    }

    float GetVelX(EntityID id) const
    {
        return velX_[id];
    }

    float GetVelZ(EntityID id) const
    {
        return velZ_[id];
    }

    float GetRadius(EntityID id) const
    {
        return radius_[id];
    }

    float GetInvMass(EntityID id) const
    {
        return invMass_[id];
    }


private:

    EntityManager() = default;

    std::vector<EntityID> entities_;

    std::vector<float> posX_;
    std::vector<float> posZ_;

    std::vector<float> velX_;
    std::vector<float> velZ_;

    std::vector<float> radius_;
    std::vector<float> invMass_;
};

}
