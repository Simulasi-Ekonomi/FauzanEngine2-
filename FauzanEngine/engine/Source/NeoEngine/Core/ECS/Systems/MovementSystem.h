#pragma once

#include "../ISystem.h"
#include "../Registry.h"
#include "../../Component/Position.h"
#include "../../Component/Velocity.h"

namespace NeoEngine
{

class MovementSystem : public ISystem
{
public:
    explicit MovementSystem(Registry& registry)
        : registry(registry)
    {}

    void Update(float dt) override;

private:
    Registry& registry;
};

} // namespace NeoEngine
