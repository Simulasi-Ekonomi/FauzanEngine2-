#include "Demos/FlipbookSurfaceDemo.h"

#include "Runtime/FlipbookPlayback.h"
#include "Runtime/FlipbookFrameSelector.h"
#include "Runtime/RenderCamera.h"
#include "Runtime/SceneSpriteAdapter.h"
#include "Runtime/SceneWorld.h"
#include "Runtime/SoftwareRenderer.h"
#include "Runtime/SoftwareSurfacePresenter.h"
#include "Runtime/SpriteBatch.h"

#include <array>

namespace NeoEngine {
namespace {
constexpr uint32_t kClear = 0xFF101420U;
constexpr uint64_t kFnvOffset = 1469598103934665603ULL;
constexpr uint64_t kFnvPrime = 1099511628211ULL;
}

bool RunFlipbookSurfaceDemo(const FlipbookSurfaceDemoConfig& config, FlipbookSurfaceDemoReceipt& receipt, FlipbookSurfaceDemoError& error) {
    receipt = {}; error = FlipbookSurfaceDemoError::None;
    if (config.width < 32U || config.height < 32U || config.width > 1024U || config.height > 1024U || config.frames != 4U || config.ppmPath.empty() || config.ppmPath.size() > 256U) { error = FlipbookSurfaceDemoError::InvalidConfiguration; return false; }
    const CpuTextureResource atlas{"flipbook-demo.atlas", 1U, TextureSourceFormat::PpmP6, 4U, 1U, {255U,0U,0U,255U, 0U,255U,0U,255U, 0U,0U,255U,255U, 255U,255U,0U,255U}};
    SceneWorld world; SceneEntity actor{};
    if (!world.Create(actor) || !world.SetTransform(actor, {0.0F,0.0F,1.0F}) || !world.UpdateTransforms()) { error = FlipbookSurfaceDemoError::WorldFailed; return false; }
    const Transform3 before = *world.GetTransform(actor);
    SceneSpriteAdapter sprites; if (!sprites.AddStaged(actor, atlas, 1.25F, 1.25F, 0, 0, 0xFFFFFFFFU)) { error = FlipbookSurfaceDemoError::SpriteFailed; return false; }
    FlipbookFrameSelector selector; FlipbookPlayback playback;
    if (!selector.Initialize({4U,1U,1U,1U,4U}) || !playback.Initialize({1.0F,true})) { error = FlipbookSurfaceDemoError::SelectorFailed; return false; }
    SoftwareRenderer renderer; RenderCamera camera; SoftwareSurfacePresenter surface;
    if (!renderer.Initialize(config.width, config.height)) { error = FlipbookSurfaceDemoError::RendererFailed; return false; }
    if (!camera.Initialize({RenderCameraMode::Orthographic, {}, 2.0F, 60.0F, static_cast<float>(config.width)/static_cast<float>(config.height), 0.1F, 10.0F})) { error = FlipbookSurfaceDemoError::CameraFailed; return false; }
    if (!surface.Initialize({config.width, config.height, config.hiddenSurface})) { error = FlipbookSurfaceDemoError::SurfaceFailed; return false; }
    constexpr std::array<uint32_t, 4U> expected{0xFFFF0000U,0xFF00FF00U,0xFF0000FFU,0xFFFFFF00U};
    uint64_t sequenceHash = kFnvOffset; uint32_t selected = 0U;
    for (uint32_t index = 0U; index < config.frames; ++index) {
        SpriteSourceRect rect{}; float sample = 0.0F;
        if (!playback.Advance(index == 0U ? 0.0F : 0.25F, sample) || !selector.Select(sample, rect) || rect.x != index || rect.y != 0U || rect.width != 1U || rect.height != 1U) { error = FlipbookSurfaceDemoError::SelectorFailed; return false; }
        if (!renderer.Clear(kClear)) { error = FlipbookSurfaceDemoError::RenderFailed; return false; }
        SpriteBatch batch; if (!sprites.QueueFrame(world, batch, rect) || !batch.Flush(renderer, camera)) { error = FlipbookSurfaceDemoError::RenderFailed; return false; }
        uint32_t count = 0U; for (const uint32_t pixel : renderer.Pixels()) if (pixel == expected[index]) ++count;
        if (count == 0U) { error = FlipbookSurfaceDemoError::RenderFailed; return false; }
        sequenceHash = (sequenceHash ^ static_cast<uint64_t>(index + 1U)) * kFnvPrime;
        sequenceHash = (sequenceHash ^ renderer.FrameHash()) * kFnvPrime;
        ++selected;
        if (!surface.PumpEvents() || surface.CloseRequested() || !surface.Present(renderer)) { error = FlipbookSurfaceDemoError::PresentFailed; return false; }
    }
    const Transform3* after = world.GetTransform(actor);
    if (after == nullptr || after->x != before.x || after->y != before.y || after->z != before.z || after->rx != before.rx || after->ry != before.ry || after->rz != before.rz || after->sx != before.sx || after->sy != before.sy || after->sz != before.sz) { error = FlipbookSurfaceDemoError::TransformChanged; return false; }
    if (!renderer.WritePpm(config.ppmPath)) { error = FlipbookSurfaceDemoError::ArtifactFailed; return false; }
    uint32_t visible = 0U; for (const uint32_t pixel : renderer.Pixels()) if (pixel != kClear) ++visible;
    receipt = {config.frames, static_cast<uint32_t>(surface.PresentedFrameCount()), visible, sequenceHash, renderer.FrameHash(), selected};
    return true;
}

} // namespace NeoEngine
