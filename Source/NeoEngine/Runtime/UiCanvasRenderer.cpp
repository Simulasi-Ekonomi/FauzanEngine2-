#include "UiCanvasRenderer.h"

#include "BitmapTextRenderer.h"
#include "RenderCamera.h"
#include "SoftwareRenderer.h"
#include "SpriteBatch.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace NeoEngine {
bool UiCanvasRenderer::SetStyle(UiCanvasStyle style) { if (style.widgetId == 0U) { lastError_ = UiCanvasError::MissingStyle; return false; } const auto found = std::find_if(styles_.begin(), styles_.end(), [&style](const UiCanvasStyle& current) { return current.widgetId == style.widgetId; }); if (found != styles_.end()) { lastError_ = UiCanvasError::DuplicateStyle; return false; } if (styles_.size() >= kMaxStyles) { lastError_ = UiCanvasError::StyleCapacity; return false; } styles_.push_back(style); lastError_ = UiCanvasError::None; return true; }
bool UiCanvasRenderer::SetLabel(UiCanvasLabel label) {
    const bool validText = !label.text.empty() && label.text.size() <= BitmapTextRenderer::kMaxCharacters && std::all_of(label.text.begin(), label.text.end(), [](char character) { return character == ' ' || (character >= 'A' && character <= 'Z') || (character >= '0' && character <= '9'); });
    if (label.widgetId == 0U || !validText || label.pixelScale == 0U || label.pixelScale > 16U) { lastError_ = UiCanvasError::InvalidLabel; return false; }
    const auto found = std::find_if(labels_.begin(), labels_.end(), [&label](const UiCanvasLabel& current) { return current.widgetId == label.widgetId; });
    if (found != labels_.end()) { lastError_ = UiCanvasError::DuplicateLabel; return false; }
    if (labels_.size() >= kMaxStyles) { lastError_ = UiCanvasError::LabelCapacity; return false; }
    labels_.push_back(std::move(label)); lastError_ = UiCanvasError::None; return true;
}
bool UiCanvasRenderer::Draw(const UiInputRouter& router, SoftwareRenderer& renderer) {
    if (renderer.Width() == 0U || renderer.Height() == 0U) { lastError_ = UiCanvasError::OutsideSurface; return false; }
    RenderCamera camera; const float width = static_cast<float>(renderer.Width()), height = static_cast<float>(renderer.Height());
    if (!camera.Initialize({RenderCameraMode::Orthographic, {width * 0.5F, height * 0.5F, 0.0F}, height * 0.5F, 60.0F, width / height, 0.1F, 10.0F})) { lastError_ = UiCanvasError::DrawFailed; return false; }
    RenderPoint3 panelDepth{}; if (!camera.Project({0.0F, 0.0F, 1.0F}, panelDepth)) { lastError_ = UiCanvasError::DrawFailed; return false; }
    for (const UiWidgetSpec& widget : router.RenderableWidgets()) {
        const auto style = std::find_if(styles_.begin(), styles_.end(), [&widget](const UiCanvasStyle& current) { return current.widgetId == widget.id; });
        if (style == styles_.end()) { lastError_ = UiCanvasError::MissingStyle; return false; }
        const UiRect& rect = widget.rect;
        if (!std::isfinite(rect.x) || !std::isfinite(rect.y) || !std::isfinite(rect.width) || !std::isfinite(rect.height) || rect.x < 0.0F || rect.y < 0.0F || rect.x + rect.width > width || rect.y + rect.height > height) { lastError_ = UiCanvasError::OutsideSurface; return false; }
        SpriteBatch batch;
        if (!batch.Queue({rect.x + rect.width * 0.5F, rect.y + rect.height * 0.5F, 1.0F, rect.width, rect.height, 0, 0, style->rgba}) || !batch.Flush(renderer, camera)) { lastError_ = UiCanvasError::DrawFailed; return false; }
        const auto label = std::find_if(labels_.begin(), labels_.end(), [&widget](const UiCanvasLabel& current) { return current.widgetId == widget.id; });
        if (label == labels_.end()) continue;
        const uint32_t textWidth = static_cast<uint32_t>(label->text.size()) * (BitmapTextRenderer::kGlyphWidth + 1U) * label->pixelScale - label->pixelScale;
        const uint32_t textHeight = BitmapTextRenderer::kGlyphHeight * label->pixelScale;
        const float pixelX = rect.x + static_cast<float>(label->insetX), pixelY = rect.y + static_cast<float>(label->insetY);
        if (pixelX != std::floor(pixelX) || pixelY != std::floor(pixelY) || pixelX < 0.0F || pixelY < 0.0F || pixelX > static_cast<float>(std::numeric_limits<uint16_t>::max()) || pixelY > static_cast<float>(std::numeric_limits<uint16_t>::max()) || pixelX + textWidth > rect.x + rect.width || pixelY + textHeight > rect.y + rect.height) { lastError_ = UiCanvasError::LabelOutsideWidget; return false; }
        BitmapTextRenderer textRenderer;
        if (!textRenderer.DrawAtDepth(renderer, label->text, static_cast<uint16_t>(pixelX), static_cast<uint16_t>(pixelY), label->pixelScale, label->rgba, panelDepth.z)) { lastError_ = UiCanvasError::DrawFailed; return false; }
    }
    lastError_ = UiCanvasError::None; return true;
}
} // namespace NeoEngine
