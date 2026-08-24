#pragma once

#include "UiInputRouter.h"

#include <cstdint>
#include <string>
#include <vector>

namespace NeoEngine {
class SoftwareRenderer;
enum class UiCanvasError : uint8_t { None, StyleCapacity, LabelCapacity, DuplicateStyle, DuplicateLabel, MissingStyle, InvalidLabel, LabelOutsideWidget, OutsideSurface, DrawFailed };
struct UiCanvasStyle { uint16_t widgetId = 0; uint32_t rgba = 0xFFFFFFFFU; };
struct UiCanvasLabel { uint16_t widgetId = 0; std::string text{}; uint16_t insetX = 0; uint16_t insetY = 0; uint8_t pixelScale = 1; uint32_t rgba = 0xFFFFFFFFU; };
class UiCanvasRenderer {
public:
    static constexpr uint16_t kMaxStyles = UiInputRouter::kMaxWidgets;
    bool SetStyle(UiCanvasStyle style);
    bool SetLabel(UiCanvasLabel label);
    bool Draw(const UiInputRouter& router, SoftwareRenderer& renderer);
    [[nodiscard]] UiCanvasError LastError() const { return lastError_; }
private:
    std::vector<UiCanvasStyle> styles_;
    std::vector<UiCanvasLabel> labels_;
    UiCanvasError lastError_ = UiCanvasError::None;
};
} // namespace NeoEngine
