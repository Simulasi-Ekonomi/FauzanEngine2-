#include "RendererCapability.h"
namespace NeoEngine {
RendererCapability RendererCapabilityProbe::Query() {
    return {RendererCapabilityState::ReadyPresent, "software-raster+sdl-surface", "bounded_cpu_rasterizer_linked;_sdl_surface_presentation_requires_runtime_opt_in"};
}
bool RendererCapabilityProbe::CanRender() { const auto state=Query().state; return state==RendererCapabilityState::ReadyHeadless||state==RendererCapabilityState::ReadyPresent; }
bool RendererCapabilityProbe::CanPresent() { return Query().state == RendererCapabilityState::ReadyPresent; }
} // namespace NeoEngine
