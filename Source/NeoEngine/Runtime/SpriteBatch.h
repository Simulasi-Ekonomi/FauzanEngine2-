#pragma once

#include "RenderCamera.h"

#include <cstdint>
#include <vector>

namespace NeoEngine {
class SoftwareRenderer;
struct CpuTextureResource;
enum class SpriteBatchError : uint8_t { None, Capacity, InvalidSize, ProjectionFailed, DrawFailed };
struct SpriteDraw { float x = 0.0F, y = 0.0F, z = 1.0F, width = 1.0F, height = 1.0F; int16_t layer = 0; int16_t order = 0; uint32_t rgba = 0xFFFFFFFFU; const CpuTextureResource* texture = nullptr; };
class SpriteBatch { public: static constexpr uint16_t kMaxSprites = 2048; bool Queue(SpriteDraw draw); bool Flush(SoftwareRenderer& renderer, RenderCamera& camera); void Clear(); [[nodiscard]] uint16_t Count() const { return static_cast<uint16_t>(sprites_.size()); } [[nodiscard]] SpriteBatchError LastError() const { return lastError_; } private: struct Pending { SpriteDraw draw{}; uint16_t sequence = 0; }; std::vector<Pending> sprites_; uint16_t sequence_ = 0; SpriteBatchError lastError_ = SpriteBatchError::None; };
} // namespace NeoEngine
