#include "Core/ECS/ArchetypeManager.h"
#include <cassert>
#include <cmath>

int main() {
    NeoEngine::ArchetypeManager ecs;
    const auto id = ecs.CreateEntity(NeoEngine::COMP_POSITION | NeoEngine::COMP_ROTATION);
    assert(ecs.HasEntity(id));
    ecs.SetPosX(id, 1.0F); ecs.SetPosY(id, 2.0F); ecs.SetPosZ(id, 3.0F);
    ecs.SetTransform(id, 1.0F, 2.0F, 3.0F, 0.0F, 0.5F, 0.0F, 2.0F, 3.0F, 4.0F);
    ecs.SetComponentMask(id, NeoEngine::COMP_POSITION | NeoEngine::COMP_ROTATION | NeoEngine::COMP_MESH);
    ecs.SetMeshAssetIdentity(id, 0x1122334455667788ULL, 0x8877665544332211ULL);
    const auto before = ecs.GetPhysicsRevision();
    ecs.SetComponentMask(id, NeoEngine::COMP_POSITION | NeoEngine::COMP_ROTATION | NeoEngine::COMP_VELOCITY | NeoEngine::COMP_COLLIDER);
    assert(ecs.HasEntity(id));
    float x=0,y=0,z=0;
    assert(ecs.TryGetPosition(id,x,y,z));
    assert(x==1.0F && y==2.0F && z==3.0F);
    assert(ecs.TryGetScale(id,x,y,z));
    assert(x==2.0F && y==3.0F && z==4.0F);
    assert(ecs.GetPhysicsRevision() > before);
    uint64_t meshHash=0U, materialHash=0U;
    assert(ecs.TryGetMeshAssetIdentity(id, meshHash, materialHash));
    assert(meshHash==0x1122334455667788ULL && materialHash==0x8877665544332211ULL);
    ecs.SetComponentMask(id, NeoEngine::COMP_POSITION | NeoEngine::COMP_ROTATION | NeoEngine::COMP_VELOCITY | NeoEngine::COMP_COLLIDER);
    assert(!ecs.TryGetMeshAssetIdentity(id, meshHash, materialHash));
    ecs.SetComponentMask(id, NeoEngine::COMP_POSITION | NeoEngine::COMP_ROTATION | NeoEngine::COMP_MESH);
    ecs.SetMeshAssetIdentity(id, 0x1122334455667788ULL, 0x8877665544332211ULL);
    assert(ecs.TryGetMeshAssetIdentity(id, meshHash, materialHash));
    assert(meshHash==0x1122334455667788ULL && materialHash==0x8877665544332211ULL);
    ecs.SetVelX(id,4.0F); ecs.SetVelY(id,5.0F); ecs.SetVelZ(id,6.0F);
    assert(ecs.TryGetVelocity(id,x,y,z) && x==4.0F && y==5.0F && z==6.0F);
    return 0;
}
