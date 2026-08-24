#include "SpriteBatch.h"

#include "SoftwareRenderer.h"

#include <algorithm>
#include <cmath>

namespace NeoEngine {
bool SpriteBatch::Queue(SpriteDraw draw) { if (sprites_.size() >= kMaxSprites) { lastError_ = SpriteBatchError::Capacity; return false; } if (!(draw.width > 0.0F) || !(draw.height > 0.0F) || !std::isfinite(draw.x) || !std::isfinite(draw.y) || !std::isfinite(draw.z) || !std::isfinite(draw.width) || !std::isfinite(draw.height)) { lastError_ = SpriteBatchError::InvalidSize; return false; } sprites_.push_back({draw, sequence_++}); lastError_ = SpriteBatchError::None; return true; }
bool SpriteBatch::Flush(SoftwareRenderer& renderer, RenderCamera& camera) { std::stable_sort(sprites_.begin(), sprites_.end(), [](const Pending& a, const Pending& b) { return a.draw.layer != b.draw.layer ? a.draw.layer < b.draw.layer : (a.draw.order != b.draw.order ? a.draw.order < b.draw.order : a.sequence < b.sequence); }); for (const Pending& pending : sprites_) { const SpriteDraw& draw = pending.draw; const float hx = draw.width * 0.5F, hy = draw.height * 0.5F; RenderPoint3 a{}, b{}, c{}, d{}; if (!camera.Project({draw.x-hx, draw.y-hy, draw.z}, a) || !camera.Project({draw.x+hx, draw.y-hy, draw.z}, b) || !camera.Project({draw.x+hx, draw.y+hy, draw.z}, c) || !camera.Project({draw.x-hx, draw.y+hy, draw.z}, d)) { lastError_ = SpriteBatchError::ProjectionFailed; return false; } const RenderVertex va{a.x,a.y,a.z,draw.rgba}, vb{b.x,b.y,b.z,draw.rgba}, vc{c.x,c.y,c.z,draw.rgba}, vd{d.x,d.y,d.z,draw.rgba}; if (!renderer.DrawTriangle(va,vb,vc) || !renderer.DrawTriangle(va,vc,vd)) { lastError_ = SpriteBatchError::DrawFailed; return false; } } lastError_ = SpriteBatchError::None; return true; }
void SpriteBatch::Clear() { sprites_.clear(); sequence_ = 0; lastError_ = SpriteBatchError::None; }
} // namespace NeoEngine
