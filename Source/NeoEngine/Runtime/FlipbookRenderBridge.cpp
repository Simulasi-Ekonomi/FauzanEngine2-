#include "Runtime/FlipbookRenderBridge.h"

#include "Runtime/SceneSpriteAdapter.h"
#include "Runtime/SceneWorld.h"
#include "Runtime/SpriteBatch.h"

namespace NeoEngine {
bool FlipbookRenderBridge::AdvanceQueue(FlipbookPlayback& playback, const FlipbookFrameSelector& selector, const SceneWorld& world, const SceneSpriteAdapter& sprites, SpriteBatch& batch, float deltaSeconds, SpriteSourceRect& output) {
    FlipbookPlayback candidatePlayback = playback; SpriteBatch candidateBatch = batch; SpriteSourceRect candidateRect = output; float sample = 0.0F;
    if (!candidatePlayback.Advance(deltaSeconds, sample)) { lastError_ = FlipbookRenderBridgeError::PlaybackFailed; return false; }
    if (!selector.Select(sample, candidateRect)) { lastError_ = FlipbookRenderBridgeError::SelectorFailed; return false; }
    if (!sprites.QueueFrame(world, candidateBatch, candidateRect)) { lastError_ = FlipbookRenderBridgeError::QueueFailed; return false; }
    playback = candidatePlayback; batch = std::move(candidateBatch); output = candidateRect; lastError_ = FlipbookRenderBridgeError::None; return true;
}
} // namespace NeoEngine
