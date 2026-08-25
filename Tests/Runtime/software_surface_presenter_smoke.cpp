#include "Runtime/NeoRuntime.h"
#include "Runtime/SoftwareRenderer.h"
#include "Runtime/SoftwareSurfacePresenter.h"

#include <SDL.h>
#include <cstdio>

int main() {
    using namespace NeoEngine;
    const auto require = [](bool condition, const char* stage) { if (!condition) std::fprintf(stderr, "SOFTWARE_SURFACE_PRESENTER_SMOKE_FAIL stage=%s\n", stage); return condition; };
    SoftwareSurfacePresenter presenter;
    if (!require(!presenter.Initialize({0, 8, true}) && presenter.LastError() == SoftwareSurfacePresenterError::InvalidConfiguration, "invalid-config")) return 1;
    SoftwareRenderer source;
    if (!require(source.Initialize(16, 16) && source.Clear(0xFF2962FFU) && presenter.Initialize({16, 16, true}) && presenter.Present(source) && presenter.PresentedFrameCount() == 1U && presenter.LastPresentedHash() == source.FrameHash(), "direct-first-present")) return 1;
    const uint64_t firstHash = presenter.LastPresentedHash();
    SoftwareRenderer wrongSize;
    if (!require(wrongSize.Initialize(8, 8) && !presenter.Present(wrongSize) && presenter.LastError() == SoftwareSurfacePresenterError::DimensionMismatch && presenter.PresentedFrameCount() == 1U && presenter.LastPresentedHash() == firstHash, "dimension-rejection")) return 1;
    if (!require(source.Clear(0xFFE53935U) && presenter.Present(source) && presenter.PresentedFrameCount() == 2U && presenter.LastPresentedHash() != firstHash, "direct-second-present")) return 1;
    const uint64_t closeFrames = presenter.PresentedFrameCount(), closeHash = presenter.LastPresentedHash();
    SDL_Event closeEvent{};
    closeEvent.type = SDL_QUIT;
    if (!require(SDL_PushEvent(&closeEvent) == 1 && presenter.PumpEvents() && presenter.CloseRequested() && !presenter.Present(source) && presenter.LastError() == SoftwareSurfacePresenterError::CloseRequested && presenter.PresentedFrameCount() == closeFrames && presenter.LastPresentedHash() == closeHash, "close-request")) return 1;
    presenter.Reset();
    if (!require(!presenter.IsReady() && !presenter.CloseRequested() && !presenter.PumpEvents() && presenter.LastError() == SoftwareSurfacePresenterError::NotInitialized, "reset")) return 1;

    NeoRuntime runtime;
    RuntimeConfig config{};
    config.farmWidth = 4;
    config.farmHeight = 4;
    config.renderWidth = 32;
    config.renderHeight = 32;
    config.farmNpcCount = 1;
    config.authoringWorldSide = 32;
    config.enableSoftwareSurfacePresentation = true;
    config.softwareSurfaceHidden = true;
    if (!require(runtime.Initialize(config) && runtime.Tick() && runtime.RenderFarm() && runtime.SurfacePresenter() != nullptr && runtime.SurfacePresenter()->PresentedFrameCount() == 1U && runtime.SurfacePresenter()->LastPresentedHash() != 0U && runtime.Shutdown(), "neo-runtime-hook")) return 1;
    std::printf("SOFTWARE_SURFACE_PRESENTER_SMOKE_OK directFrames=2 closeRequest=1 runtimePresent=1 hiddenSurface=1\n");
    return 0;
}
