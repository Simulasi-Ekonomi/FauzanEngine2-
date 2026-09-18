#include "Core/ECS/ArchetypeManager.h"
#include <cassert>
#include <cmath>
#include <limits>

int main() {
    NeoEngine::ArchetypeManager ecs;
    const auto a = ecs.CreateEntity(NeoEngine::COMP_POSITION | NeoEngine::COMP_VELOCITY | NeoEngine::COMP_ROTATION);
    const auto b = ecs.CreateEntity(NeoEngine::COMP_POSITION | NeoEngine::COMP_VELOCITY | NeoEngine::COMP_ROTATION);
    assert(a != std::numeric_limits<NeoEngine::EntityID>::max());
    assert(b != std::numeric_limits<NeoEngine::EntityID>::max());
    assert(ecs.EntityCount() == 2U);
    ecs.SetPosX(a, 3.0F); ecs.SetPosY(a, 4.0F); ecs.SetPosZ(a, 5.0F);
    ecs.SetVelX(a, 1.0F); ecs.SetVelY(a, 2.0F); ecs.SetVelZ(a, 3.0F);
    ecs.SetRotX(a, 0.1F); ecs.SetRotY(a, 0.2F); ecs.SetRotZ(a, 0.3F);
    float x=0.0F,y=0.0F,z=0.0F;
    assert(ecs.TryGetPosition(a,x,y,z) && x==3.0F && y==4.0F && z==5.0F);
    assert(ecs.TryGetVelocity(a,x,y,z) && x==1.0F && y==2.0F && z==3.0F);
    assert(ecs.TryGetRotation(a,x,y,z));
    assert(std::fabs(x-0.1F)<1e-6F && std::fabs(y-0.2F)<1e-6F && std::fabs(z-0.3F)<1e-6F);
    ecs.DestroyEntity(a);
    assert(!ecs.HasEntity(a) && ecs.HasEntity(b) && ecs.EntityCount()==1U);
    assert(!ecs.TryGetPosition(a,x,y,z));
    return 0;
}
