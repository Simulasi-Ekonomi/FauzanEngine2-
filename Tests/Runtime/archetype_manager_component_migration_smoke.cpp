#include "Core/ECS/ArchetypeManager.h"
#include <cmath>
#include <cstdio>

#define REQUIRE(...) do { if (!(__VA_ARGS__)) { std::fprintf(stderr, "MIGRATION_REQUIRE_FAIL:" #__VA_ARGS__ "\n"); return 1; } } while (false)

int main() {
    NeoEngine::ArchetypeManager ecs;
    const auto id = ecs.CreateEntity(NeoEngine::COMP_POSITION | NeoEngine::COMP_ROTATION);
    REQUIRE(ecs.HasEntity(id));
    ecs.SetPosX(id, 1.0F); ecs.SetPosY(id, 2.0F); ecs.SetPosZ(id, 3.0F);
    ecs.SetTransform(id, 1.0F, 2.0F, 3.0F, 0.0F, 0.5F, 0.0F, 2.0F, 3.0F, 4.0F);
    ecs.SetComponentMask(id, NeoEngine::COMP_POSITION | NeoEngine::COMP_ROTATION | NeoEngine::COMP_MESH | NeoEngine::COMP_VELOCITY);
    ecs.SetMeshAssetIdentity(id, 0x1122334455667788ULL, 0x8877665544332211ULL);
    const auto before = ecs.GetPhysicsRevision();
    ecs.SetComponentMask(id, NeoEngine::COMP_POSITION | NeoEngine::COMP_ROTATION | NeoEngine::COMP_VELOCITY | NeoEngine::COMP_COLLIDER);
    REQUIRE(ecs.HasEntity(id));
    float x=0,y=0,z=0;
    REQUIRE(ecs.TryGetPosition(id,x,y,z));
    REQUIRE(x==1.0F && y==2.0F && z==3.0F);
    REQUIRE(ecs.TryGetScale(id,x,y,z));
    REQUIRE(x==2.0F && y==3.0F && z==4.0F);
    REQUIRE(ecs.GetPhysicsRevision() > before);
    uint64_t meshHash=0U, materialHash=0U;
    REQUIRE(ecs.TryGetMeshAssetIdentity(id, meshHash, materialHash));
    REQUIRE(meshHash==0x1122334455667788ULL && materialHash==0x8877665544332211ULL);
    ecs.SetComponentMask(id, NeoEngine::COMP_POSITION | NeoEngine::COMP_ROTATION | NeoEngine::COMP_MESH);
    ecs.SetMeshAssetIdentity(id, 0x1122334455667788ULL, 0x8877665544332211ULL);
    REQUIRE(ecs.TryGetMeshAssetIdentity(id, meshHash, materialHash));
    REQUIRE(meshHash==0x1122334455667788ULL && materialHash==0x8877665544332211ULL);
    ecs.SetVelX(id,4.0F); ecs.SetVelY(id,5.0F); ecs.SetVelZ(id,6.0F);
    REQUIRE(ecs.TryGetVelocity(id,x,y,z) && x==4.0F && y==5.0F && z==6.0F);
    return 0;
}
