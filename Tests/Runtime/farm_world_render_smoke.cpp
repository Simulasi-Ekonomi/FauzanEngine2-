#include "Runtime/FarmRenderAdapter.h"
#include "Runtime/RenderCamera.h"
#include "Runtime/SoftwareRenderer.h"
#include "Systems/FarmWorldTool.h"
#include "Systems/TrustSafetySystem.h"

#include <cstdio>

int main() {
    NeoEngine::FarmSystem farm(20, 20, 100);
    NeoEngine::TrustSafetySystem trust;
    NeoEngine::FarmWorldTool world;
    NeoEngine::FarmWorldConfig config{};
    config.worldWidth = 20;
    config.worldHeight = 20;
    config.npcCount = 10;
    if (!world.Initialize(farm, trust, "farm-render-player", config) ||
        !world.SetGovernmentPolicy(NeoEngine::FarmGovernmentPolicy::ConstructionPermits, true)) return 1;
    uint64_t permit = 0;
    uint32_t building = 0;
    if (!world.IssueBuildingPermit(NeoEngine::FarmBuildingType::TownHall, permit) || !world.PlaceBuilding(permit, 1, 1, building) ||
        !world.SetCharacterState({3, 3, 2}) || !world.Tick(5)) return 1;
    NeoEngine::SoftwareRenderer first;
    NeoEngine::SoftwareRenderer second;
    NeoEngine::RenderCamera camera;
    if (!first.Initialize(128, 128) || !second.Initialize(128, 128) ||
        !camera.Initialize({NeoEngine::RenderCameraMode::Orthographic, {}, 10.0F, 60.0F, 1.0F, 0.1F, 10.0F}) ||
        !NeoEngine::FarmRenderAdapter::RenderWorld(farm, world, first) || !second.Clear(0xFF17324D) ||
        !NeoEngine::FarmRenderAdapter::RenderWorldTiles(farm, second, camera) || !NeoEngine::FarmRenderAdapter::RenderWorldActors(farm, world, second, camera) ||
        first.FrameHash() == 0 || first.FrameHash() != second.FrameHash() || !first.WritePpm("farm_world_render_smoke.ppm")) return 1;
    std::printf("FARM_WORLD_RENDER_SMOKE_OK hash=%llu cameraSprites=1 buildings=1 npcs=10\n", static_cast<unsigned long long>(first.FrameHash()));
    return 0;
}
