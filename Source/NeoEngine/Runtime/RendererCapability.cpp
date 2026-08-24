#include "RendererCapability.h"
namespace NeoEngine {
RendererCapability RendererCapabilityProbe::Query() {
    return {RendererCapabilityState::ReadyHeadless, "software-raster", "bounded_cpu_rasterizer_linked;_gpu_surface_presentation_not_implemented"};
}
bool RendererCapabilityProbe::CanRender() { const auto state=Query().state; return state==RendererCapabilityState::ReadyHeadless||state==RendererCapabilityState::ReadyPresent; }
bool RendererCapabilityProbe::CanPresent() { return Query().state == RendererCapabilityState::ReadyPresent; }
} // namespace NeoEngine
