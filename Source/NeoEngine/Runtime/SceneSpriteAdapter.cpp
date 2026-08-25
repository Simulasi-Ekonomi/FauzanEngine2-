#include "Runtime/SceneSpriteAdapter.h"

#include <cmath>

namespace NeoEngine {
namespace { bool ValidTexture(const CpuTextureResource& texture) { return !texture.assetId.empty() && texture.sourceHash != 0U && texture.width > 0U && texture.height > 0U && texture.rgba.size() == static_cast<size_t>(texture.width) * texture.height * 4U; } }
bool SceneSpriteAdapter::Fail(SceneSpriteAdapterError error) const { lastError_ = error; return false; }
bool SceneSpriteAdapter::AddStaged(SceneEntity entity, const CpuTextureResource& texture, float width, float height, int16_t layer, int16_t order, uint32_t rgba, float rotationRadians, bool faceCamera, bool depthWrite) { if (instances_.size() >= kMaxInstances) return Fail(SceneSpriteAdapterError::Capacity); if (!ValidTexture(texture) || !std::isfinite(width) || !std::isfinite(height) || !std::isfinite(rotationRadians) || width <= 0.0F || height <= 0.0F) return Fail(SceneSpriteAdapterError::InvalidResource); for (const Instance& instance : instances_) if (instance.entity == entity) return Fail(SceneSpriteAdapterError::DuplicateEntity); instances_.push_back({entity, texture, width, height, layer, order, rgba, rotationRadians, faceCamera, depthWrite}); lastError_ = SceneSpriteAdapterError::None; return true; }
bool SceneSpriteAdapter::Queue(const SceneWorld& world, SpriteBatch& batch) const { for (const Instance& instance : instances_) { const Transform3* transform = world.GetTransform(instance.entity); if (transform == nullptr) return Fail(SceneSpriteAdapterError::MissingTransform); if (!batch.Queue({transform->x, transform->y, transform->z, instance.width * transform->sx, instance.height * transform->sy, instance.layer, instance.order, instance.rgba, &instance.texture, transform->rz + instance.rotationRadians, instance.faceCamera, instance.depthWrite})) return Fail(SceneSpriteAdapterError::QueueRejected); } lastError_ = SceneSpriteAdapterError::None; return true; }
void SceneSpriteAdapter::Clear() { instances_.clear(); lastError_ = SceneSpriteAdapterError::None; }
} // namespace NeoEngine
