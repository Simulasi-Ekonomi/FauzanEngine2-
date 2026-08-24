#pragma once
#include "Core/ECS/ECSCore.h"

namespace NeoEngine {
class MovementSystem {
public:
    void Update(float dt, Registry& registry);
};
}
