#pragma once
#include "ECSCore.h"

namespace NeoEngine {

struct Position { float x, y, z; };
struct Velocity { float vx, vy, vz; };

class MovementSystem {
public:
    void Update(float dt, Registry& registry);
};

} // namespace
