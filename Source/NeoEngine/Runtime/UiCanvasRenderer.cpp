#include "UiCanvasRenderer.h"

#include "AssetRegistry.h"
#include "BitmapTextRenderer.h"
#include "RenderCamera.h"
#include "SoftwareRenderer.h"
#include "SpriteBatch.h"
#include "TextureStaging.h"

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
bool UiCanvasRenderer::SetImage(const AssetRegistry& registry, UiCanvasImage image) {
    const CpuTextureResource* texture = image.texture;
    const AssetDefinition* definition = texture == nullptr ? nullptr : registry.Find(texture->assetId);
    const bool validTexture = texture != nullptr && !texture->assetId.empty() && texture->sourceHash != 0U && texture->width != 0U && texture->height != 0U && texture->rgba.size() == static_cast<size_t>(texture->width) * texture->height * 4U && definition != nullptr && definition->kind == AssetKind::Texture && definition->state == AssetState::Ready && definition->contentHash == texture->sourceHash;
    const bool fullImage = image.sourceX == 0U && image.sourceY == 0U && image.sourceWidth == 0U && image.sourceHeight == 0U;
    const bool validSource = fullImage || (image.sourceWidth != 0U && image.sourceHeight != 0U && image.sourceX < texture->width && image.sourceY < texture->height && static_cast<uint32_t>(image.sourceX) + image.sourceWidth <= texture->width && static_cast<uint32_t>(image.sourceY) + image.sourceHeight <= texture->height);
    if (image.widgetId == 0U || !validTexture || !validSource) { lastError_ = UiCanvasError::InvalidImage; return false; }
    if (std::any_of(images_.begin(), images_.end(), [&image](const UiCanvasImage& current) { return current.widgetId == image.widgetId; })) { lastError_ = UiCanvasError::DuplicateImage; return false; }
    if (images_.size() >= kMaxStyles) { lastError_ = UiCanvasError::ImageCapacity; return false; }
    image.registry = &registry; image.expectedHash = texture->sourceHash; images_.push_back(image); lastError_ = UiCanvasError::None; return true;
}
bool UiCanvasRenderer::Draw(const UiInputRouter& router, SoftwareRenderer& renderer) {
    if (renderer.Width() == 0U || renderer.Height() == 0U) { lastError_ = UiCanvasError::OutsideSurface; return false; }
    RenderCamera camera; const float width = static_cast<float>(renderer.Width()), height = static_cast<float>(renderer.Height());
    if (!camera.Initialize({RenderCameraMode::Orthographic, {width * 0.5F, height * 0.5F, 0.0F}, height * 0.5F + 1e-3F, 60.0F, width / height, 0.1F, 10.0F})) { lastError_ = UiCanvasError::DrawFailed; return false; }
    RenderPoint3 panelDepth{}; if (!camera.Project({0.0F, 0.0F, 0.5F}, panelDepth)) { lastError_ = UiCanvasError::DrawFailed; return false; }
    const std::vector<UiWidgetSpec> widgets = router.RenderableWidgets();
    for (const UiCanvasImage& image : images_) {
        const bool widgetExists = std::any_of(widgets.begin(), widgets.end(), [&image](const UiWidgetSpec& widget) { return widget.id == image.widgetId; });
        const AssetDefinition* definition = image.registry == nullptr || image.texture == nullptr ? nullptr : image.registry->Find(image.texture->assetId);
        const bool currentTexture = image.texture != nullptr && image.texture->sourceHash == image.expectedHash && image.texture->width != 0U && image.texture->height != 0U && image.texture->rgba.size() == static_cast<size_t>(image.texture->width) * image.texture->height * 4U && definition != nullptr && definition->kind == AssetKind::Texture && definition->state == AssetState::Ready && definition->contentHash == image.expectedHash;
        if (!widgetExists) { lastError_ = UiCanvasError::MissingImageWidget; return false; }
        if (!currentTexture) { lastError_ = UiCanvasError::InvalidImage; return false; }
    }
    SoftwareRenderer candidate = renderer;
    for (const UiWidgetSpec& widget : widgets) {
        const auto style = std::find_if(styles_.begin(), styles_.end(), [&widget](const UiCanvasStyle& current) { return current.widgetId == widget.id; });
        if (style == styles_.end()) { lastError_ = UiCanvasError::MissingStyle; return false; }
        const UiRect& rect = widget.rect;
        if (!std::isfinite(rect.x) || !std::isfinite(rect.y) || !std::isfinite(rect.width) || !std::isfinite(rect.height) || rect.x < 0.0F || rect.y < 0.0F || rect.x + rect.width > width || rect.y + rect.height > height) { lastError_ = UiCanvasError::OutsideSurface; return false; }
        SpriteBatch batch;
        if (!batch.Queue({rect.x + rect.width * 0.5F, rect.y + rect.height * 0.5F, 1.0F, rect.width, rect.height, 0, 0, style->rgba}) || !batch.Flush(candidate, camera)) { lastError_ = UiCanvasError::DrawFailed; return false; }
        const auto image = std::find_if(images_.begin(), images_.end(), [&widget](const UiCanvasImage& current) { return current.widgetId == widget.id; });
        if (image != images_.end()) {
            SpriteBatch imageBatch;
            if (!imageBatch.Queue({rect.x + rect.width * 0.5F, rect.y + rect.height * 0.5F, 0.99F, rect.width, rect.height, 0, 0, image->rgba, image->texture, 0.0F, false, false, image->sourceX, image->sourceY, image->sourceWidth, image->sourceHeight}) || !imageBatch.Flush(candidate, camera)) { lastError_ = UiCanvasError::DrawFailed; return false; }
        }
        const auto label = std::find_if(labels_.begin(), labels_.end(), [&widget](const UiCanvasLabel& current) { return current.widgetId == widget.id; });
        if (label == labels_.end()) continue;
        const uint32_t textWidth = static_cast<uint32_t>(label->text.size()) * (BitmapTextRenderer::kGlyphWidth + 1U) * label->pixelScale - label->pixelScale;
        const uint32_t textHeight = BitmapTextRenderer::kGlyphHeight * label->pixelScale;
        const float pixelX = rect.x + static_cast<float>(label->insetX), pixelY = rect.y + static_cast<float>(label->insetY);
        if (pixelX != std::floor(pixelX) || pixelY != std::floor(pixelY) || pixelX < 0.0F || pixelY < 0.0F || pixelX > static_cast<float>(std::numeric_limits<uint16_t>::max()) || pixelY > static_cast<float>(std::numeric_limits<uint16_t>::max()) || pixelX + textWidth > rect.x + rect.width || pixelY + textHeight > rect.y + rect.height) { lastError_ = UiCanvasError::LabelOutsideWidget; return false; }
        BitmapTextRenderer textRenderer;
        if (!textRenderer.DrawAtDepth(candidate, label->text, static_cast<uint16_t>(pixelX), static_cast<uint16_t>(pixelY), label->pixelScale, label->rgba, panelDepth.z)) { lastError_ = UiCanvasError::DrawFailed; return false; }
    }
    renderer = std::move(candidate); lastError_ = UiCanvasError::None; return true;
}
} // namespace NeoEngine
