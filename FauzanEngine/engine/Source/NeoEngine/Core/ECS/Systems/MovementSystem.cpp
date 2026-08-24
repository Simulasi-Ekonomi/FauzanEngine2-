#include "MovementSystem.h"

namespace NeoEngine
{

void MovementSystem::Update(float dt)
{
    auto entities = registry.ViewEntities<Position, Velocity>();

    for (auto e : entities)
    {
        auto& pos = registry.GetComponent<Position>(e);
        auto& vel = registry.GetComponent<Velocity>(e);

        pos.x += vel.x * dt;
        pos.y += vel.y * dt;
    }
}

} // namespace NeoEngine
