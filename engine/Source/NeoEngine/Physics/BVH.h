#pragma once

#include <vector>
#include "Collider.h"

namespace NeoEngine
{

class BVH
{
public:

    void Build(const std::vector<Collider*>& colliders);

    const std::vector<std::pair<Collider*,Collider*>>& GetPairs() const;

private:

    [[maybe_unused]] std::vector<std::pair<Collider*,Collider*>> pairs;

};

}
