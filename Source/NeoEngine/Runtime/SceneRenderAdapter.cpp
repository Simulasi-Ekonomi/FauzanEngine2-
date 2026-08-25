#include "Runtime/SceneRenderAdapter.h"

#include "Runtime/SoftwareRenderer.h"

namespace NeoEngine {
bool SceneRenderAdapter::Draw(const SceneWorld& world, SceneMeshAdapter& meshes, const SceneSpriteAdapter& sprites, RenderCamera& camera, SoftwareRenderer& renderer, const DirectionalLight& light) {
    SoftwareRenderer candidate = renderer;
    if (!meshes.Draw(world, camera, candidate, light)) { lastError_ = SceneRenderAdapterError::MeshDrawFailed; return false; }
    SpriteBatch batch;
    if (!sprites.Queue(world, batch)) { lastError_ = SceneRenderAdapterError::SpriteQueueFailed; return false; }
    if (!batch.Flush(candidate, camera)) { lastError_ = SceneRenderAdapterError::SpriteFlushFailed; return false; }
    renderer = std::move(candidate); lastError_ = SceneRenderAdapterError::None; return true;
}
} // namespace NeoEngine
