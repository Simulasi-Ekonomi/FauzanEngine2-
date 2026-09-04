#include "Runtime/SoftwareSurfacePresenter.h"

#include "Runtime/SoftwareRenderer.h"

#include <SDL3/SDL.h>

namespace NeoEngine {
bool SoftwareSurfacePresenter::Fail(SoftwareSurfacePresenterError error) { lastError_ = error; return false; }
SoftwareSurfacePresenter::~SoftwareSurfacePresenter() { Reset(); }

bool SoftwareSurfacePresenter::Initialize(SoftwareSurfacePresenterConfig config) {
    Reset();
    if (config.width == 0U || config.height == 0U || config.width > 4096U || config.height > 4096U) return Fail(SoftwareSurfacePresenterError::InvalidConfiguration);
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) return Fail(SoftwareSurfacePresenterError::SdlInitFailed);
    const SDL_WindowFlags flags = config.hidden ? SDL_WINDOW_HIDDEN : 0;
    SDL_Window* window = SDL_CreateWindow("NeoEngine Software Surface", static_cast<int>(config.width), static_cast<int>(config.height), flags);
    if (window == nullptr) { SDL_QuitSubSystem(SDL_INIT_VIDEO); return Fail(SoftwareSurfacePresenterError::WindowFailed); }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, "software");
    if (renderer == nullptr) { SDL_DestroyWindow(window); SDL_QuitSubSystem(SDL_INIT_VIDEO); return Fail(SoftwareSurfacePresenterError::RendererFailed); }
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, static_cast<int>(config.width), static_cast<int>(config.height));
    if (texture == nullptr) { SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_QuitSubSystem(SDL_INIT_VIDEO); return Fail(SoftwareSurfacePresenterError::TextureFailed); }
    window_ = window; renderer_ = renderer; texture_ = texture; width_ = config.width; height_ = config.height; windowWidth_ = config.width; windowHeight_ = config.height; presentedFrameCount_ = 0; lastPresentedHash_ = 0; closeRequested_ = false; lastError_ = SoftwareSurfacePresenterError::None; return true;
}

bool SoftwareSurfacePresenter::PumpEvents() {
    if (!IsReady()) return Fail(SoftwareSurfacePresenterError::NotInitialized);
    SDL_Event event{};
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) closeRequested_ = true;
        if (event.type == SDL_EVENT_WINDOW_RESIZED || event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
            if (event.window.data1 > 0 && event.window.data2 > 0) { windowWidth_ = static_cast<uint32_t>(event.window.data1); windowHeight_ = static_cast<uint32_t>(event.window.data2); }
        }
    }
    lastError_ = SoftwareSurfacePresenterError::None;
    return true;
}

bool SoftwareSurfacePresenter::Present(const SoftwareRenderer& source) {
    if (!IsReady()) return Fail(SoftwareSurfacePresenterError::NotInitialized);
    if (closeRequested_) return Fail(SoftwareSurfacePresenterError::CloseRequested);
    if (source.Width() != width_ || source.Height() != height_ || source.Pixels().size() != static_cast<size_t>(width_) * height_) return Fail(SoftwareSurfacePresenterError::DimensionMismatch);
    SDL_Texture* texture = static_cast<SDL_Texture*>(texture_);
    SDL_Renderer* renderer = static_cast<SDL_Renderer*>(renderer_);
    if (!SDL_UpdateTexture(texture, nullptr, source.Pixels().data(), static_cast<int>(width_ * sizeof(uint32_t))) || !SDL_RenderClear(renderer) || !SDL_RenderTexture(renderer, texture, nullptr, nullptr) || !SDL_RenderPresent(renderer)) return Fail(SoftwareSurfacePresenterError::UploadFailed);
    ++presentedFrameCount_; lastPresentedHash_ = source.FrameHash(); lastError_ = SoftwareSurfacePresenterError::None; return true;
}

void SoftwareSurfacePresenter::Reset() {
    if (texture_ != nullptr) SDL_DestroyTexture(static_cast<SDL_Texture*>(texture_));
    if (renderer_ != nullptr) SDL_DestroyRenderer(static_cast<SDL_Renderer*>(renderer_));
    if (window_ != nullptr) SDL_DestroyWindow(static_cast<SDL_Window*>(window_));
    if (window_ != nullptr || renderer_ != nullptr || texture_ != nullptr) SDL_QuitSubSystem(SDL_INIT_VIDEO);
    window_ = nullptr; renderer_ = nullptr; texture_ = nullptr; width_ = 0; height_ = 0; windowWidth_ = 0; windowHeight_ = 0; presentedFrameCount_ = 0; lastPresentedHash_ = 0; closeRequested_ = false;
}
} // namespace NeoEngine
