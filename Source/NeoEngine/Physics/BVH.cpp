#include <cassert>
#include "BVH.h"

namespace NeoEngine
{

void BVH::Build(const std::vector<Collider*>& colliders)
{
    pairs.clear();

    for(size_t i=0;i<colliders.size();i++)
    {
        for(size_t j=i+1;j<colliders.size();j++)
        {
            pairs.push_back({colliders[i],colliders[j]});
        }
    }
}

const std::vector<std::pair<Collider*,Collider*>>& BVH::GetPairs() const
{
    return pairs;
}

}
