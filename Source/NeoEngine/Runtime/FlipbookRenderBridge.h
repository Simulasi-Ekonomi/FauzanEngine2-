#pragma once

#include "FlipbookFrameSelector.h"
#include "FlipbookPlayback.h"

#include <cstdint>

namespace NeoEngine {

class SceneSpriteAdapter;
class SceneWorld;
class SpriteBatch;

enum class FlipbookRenderBridgeError : uint8_t { None, PlaybackFailed, SelectorFailed, QueueFailed };

class FlipbookRenderBridge {
public:
    bool AdvanceQueue(FlipbookPlayback& playback, const FlipbookFrameSelector& selector, const SceneWorld& world, const SceneSpriteAdapter& sprites, SpriteBatch& batch, float deltaSeconds, SpriteSourceRect& output);
    [[nodiscard]] FlipbookRenderBridgeError LastError() const { return lastError_; }
private:
    FlipbookRenderBridgeError lastError_ = FlipbookRenderBridgeError::None;
};

} // namespace NeoEngine
