#pragma once

#include "ISystem.h"
#include "Core/ECS/Registry.h"

namespace NeoEngine {

class MovementSystem : public ISystem {
public:
    void Update(float dt) override;
};

}
