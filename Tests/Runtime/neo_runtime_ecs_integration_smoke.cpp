#include "Runtime/NeoRuntime.h"
#include "Runtime/MaterialStaging.h"
#include <cmath>
#include <limits>
#define REQUIRE(...) do { if (!(__VA_ARGS__)) return 1; } while (false)

int main() {
    NeoEngine::NeoRuntime runtime;
    NeoEngine::RuntimeConfig config{};
    config.farmNpcCount = 1;
    config.renderWidth = 64;
    config.renderHeight = 48;
    REQUIRE(runtime.Initialize(config));
    REQUIRE(runtime.ECS() != nullptr);
    REQUIRE(runtime.Scene() != nullptr);
    REQUIRE(runtime.SceneECS().sceneCount == runtime.Scene()->AliveCount());
    REQUIRE(runtime.SceneECS().ecsCount == runtime.Scene()->AliveCount());
    const auto entities = runtime.Scene()->AliveEntities();
    REQUIRE(!entities.empty());
    const NeoEngine::EntityID ecsId = runtime.SceneECSId(entities.front());
    REQUIRE(ecsId != std::numeric_limits<NeoEngine::EntityID>::max());
    float x=0.0F,y=0.0F,z=0.0F,rx=0.0F,ry=0.0F,rz=0.0F,sx=0.0F,sy=0.0F,sz=0.0F;
    REQUIRE(runtime.ECS()->TryGetPosition(ecsId,x,y,z));
    REQUIRE(runtime.ECS()->TryGetRotation(ecsId,rx,ry,rz));
    REQUIRE(runtime.ECS()->TryGetScale(ecsId,sx,sy,sz));
    REQUIRE(sx==1.0F && sy==1.0F && sz==1.0F);
    REQUIRE(std::isfinite(x) && std::isfinite(y) && std::isfinite(z));
    REQUIRE(std::isfinite(rx) && std::isfinite(ry) && std::isfinite(rz));

    NeoEngine::CpuMeshResource mesh{};
    mesh.assetId = "smoke.mesh";
    mesh.sourceHash = 0x1020304050607080ULL;
    mesh.vertices = {
        {{0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F}, 0.0F, 0.0F},
        {{1.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F}, 1.0F, 0.0F},
        {{0.0F, 1.0F, 0.0F}, {0.0F, 0.0F, 1.0F}, 0.0F, 1.0F}
    };
    mesh.indices = {0U, 1U, 2U};
    NeoEngine::CpuMaterialResource material{};
    material.assetId = "smoke.material";
    material.materialName = "default";
    material.sourceHash = 0x9080706050403020ULL;
    REQUIRE(runtime.SceneMeshes()->AddStaged(entities.front(), mesh, material));

    REQUIRE(runtime.Tick());
    REQUIRE(runtime.SceneECS().sceneCount == runtime.Scene()->AliveCount());
    REQUIRE(runtime.SceneECS().ecsCount == runtime.Scene()->AliveCount());
    const NeoEngine::EntityID meshEcsId = runtime.SceneECSId(entities.front());
    REQUIRE(meshEcsId != std::numeric_limits<NeoEngine::EntityID>::max());
    REQUIRE((runtime.ECS()->GetComponentMask(meshEcsId) & NeoEngine::COMP_MESH) != 0U);
    uint64_t meshHash=0U, materialHash=0U;
    REQUIRE(runtime.ECS()->TryGetMeshAssetIdentity(meshEcsId, meshHash, materialHash));
    REQUIRE(meshHash == mesh.sourceHash && materialHash == material.sourceHash);
    REQUIRE(runtime.SceneECS().sceneCount == runtime.Scene()->AliveCount());
    REQUIRE(runtime.SceneECS().ecsCount == runtime.Scene()->AliveCount());
    REQUIRE(runtime.Shutdown());
    return 0;
}
