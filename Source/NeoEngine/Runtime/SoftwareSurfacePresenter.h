#pragma once

#include <cstdint>

namespace NeoEngine {
class SoftwareRenderer;

enum class SoftwareSurfacePresenterError : uint8_t { None, InvalidConfiguration, SdlInitFailed, WindowFailed, RendererFailed, TextureFailed, NotInitialized, DimensionMismatch, UploadFailed, CloseRequested };
struct SoftwareSurfacePresenterConfig { uint32_t width = 0; uint32_t height = 0; bool hidden = true; };

// SDL surface host for the canonical CPU renderer. It owns only presentation
// resources and never mutates SoftwareRenderer pixels or gameplay state.
class SoftwareSurfacePresenter {
public:
    ~SoftwareSurfacePresenter();
    SoftwareSurfacePresenter() = default;
    SoftwareSurfacePresenter(const SoftwareSurfacePresenter&) = delete;
    SoftwareSurfacePresenter& operator=(const SoftwareSurfacePresenter&) = delete;
    bool Initialize(SoftwareSurfacePresenterConfig config);
    bool PumpEvents();
    bool Present(const SoftwareRenderer& renderer);
    void Reset();
    [[nodiscard]] bool IsReady() const { return window_ != nullptr && renderer_ != nullptr && texture_ != nullptr; }
    [[nodiscard]] uint64_t PresentedFrameCount() const { return presentedFrameCount_; }
    [[nodiscard]] uint64_t LastPresentedHash() const { return lastPresentedHash_; }
    [[nodiscard]] bool CloseRequested() const { return closeRequested_; }
    [[nodiscard]] SoftwareSurfacePresenterError LastError() const { return lastError_; }
private:
    bool Fail(SoftwareSurfacePresenterError error);
    void* window_ = nullptr;
    void* renderer_ = nullptr;
    void* texture_ = nullptr;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    uint64_t presentedFrameCount_ = 0;
    uint64_t lastPresentedHash_ = 0;
    bool closeRequested_ = false;
    SoftwareSurfacePresenterError lastError_ = SoftwareSurfacePresenterError::NotInitialized;
};
} // namespace NeoEngine
