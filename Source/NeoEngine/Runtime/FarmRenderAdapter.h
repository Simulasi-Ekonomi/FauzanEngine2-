#pragma once

namespace NeoEngine {
class FarmSystem;
class FarmWorldTool;
class SoftwareRenderer;
class RenderCamera;

class FarmRenderAdapter {
public:
    static bool Render(const FarmSystem& farm, SoftwareRenderer& renderer);
    static bool RenderWorldTiles(const FarmSystem& farm, SoftwareRenderer& renderer, RenderCamera& camera);
    static bool RenderWorld(const FarmSystem& farm, const FarmWorldTool& world, SoftwareRenderer& renderer);
    static bool RenderWorldActors(const FarmSystem& farm, const FarmWorldTool& world, SoftwareRenderer& renderer, RenderCamera& camera);
};
}
