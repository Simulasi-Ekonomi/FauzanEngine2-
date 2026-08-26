#pragma once

#include "UiInputRouter.h"

#include <cstdint>
#include <string>
#include <vector>

namespace NeoEngine {
class AssetRegistry;
class SoftwareRenderer;
struct CpuTextureResource;
enum class UiCanvasError : uint8_t { None, StyleCapacity, LabelCapacity, ImageCapacity, DuplicateStyle, DuplicateLabel, DuplicateImage, MissingStyle, MissingImageWidget, InvalidLabel, InvalidImage, LabelOutsideWidget, OutsideSurface, DrawFailed };
struct UiCanvasStyle { uint16_t widgetId = 0; uint32_t rgba = 0xFFFFFFFFU; };
struct UiCanvasLabel { uint16_t widgetId = 0; std::string text{}; uint16_t insetX = 0; uint16_t insetY = 0; uint8_t pixelScale = 1; uint32_t rgba = 0xFFFFFFFFU; };
struct UiCanvasImage { uint16_t widgetId = 0; const AssetRegistry* registry = nullptr; const CpuTextureResource* texture = nullptr; uint64_t expectedHash = 0U; uint16_t sourceX = 0U; uint16_t sourceY = 0U; uint16_t sourceWidth = 0U; uint16_t sourceHeight = 0U; uint32_t rgba = 0xFFFFFFFFU; };
class UiCanvasRenderer {
public:
    static constexpr uint16_t kMaxStyles = UiInputRouter::kMaxWidgets;
    bool SetStyle(UiCanvasStyle style);
    bool SetLabel(UiCanvasLabel label);
    bool SetImage(const AssetRegistry& registry, UiCanvasImage image);
    bool Draw(const UiInputRouter& router, SoftwareRenderer& renderer);
    [[nodiscard]] UiCanvasError LastError() const { return lastError_; }
private:
    std::vector<UiCanvasStyle> styles_;
    std::vector<UiCanvasLabel> labels_;
    std::vector<UiCanvasImage> images_;
    UiCanvasError lastError_ = UiCanvasError::None;
};
} // namespace NeoEngine
