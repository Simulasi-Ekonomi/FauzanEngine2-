#include "Core/ECS/Systems/MovementSystem.h"
#include "Core/ECS/ECSCore.h"
#include "Core/Math/NeoMath.h"

namespace NeoEngine {

void MovementSystem::Update(float dt, Registry& registry) {
    auto entities = registry.ViewEntities<Position, Velocity>();
    for (auto e : entities) {
        auto& pos = registry.GetComponent<Position>(e);
        auto& vel = registry.GetComponent<Velocity>(e);
        pos.x += vel.vx * dt;
        pos.y += vel.vy * dt;
        pos.z += vel.vz * dt;
    }
}

} // namespace NeoEngine
