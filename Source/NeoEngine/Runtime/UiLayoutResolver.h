#pragma once

#include "UiInputRouter.h"

#include <cstdint>
#include <vector>

namespace NeoEngine {

enum class UiLayoutAnchor : uint8_t { TopLeft, TopRight, BottomLeft, BottomRight, Fill };
enum class UiLayoutFlow : uint8_t { None, VerticalStack };
enum class UiLayoutError : uint8_t { None, InvalidSurface, EmptyPlan, Capacity, DuplicateWidget, MissingWidget, ParentUnresolved, InvalidSpec, InsufficientSpace };

struct UiLayoutPadding { float left = 0.0F; float top = 0.0F; float right = 0.0F; float bottom = 0.0F; };
struct UiLayoutSurface { float width = 0.0F; float height = 0.0F; };
struct UiLayoutSpec {
    uint16_t widgetId = 0U;
    UiLayoutAnchor anchor = UiLayoutAnchor::TopLeft;
    UiLayoutFlow flow = UiLayoutFlow::None;
    UiLayoutPadding padding{};
    float width = 0.0F;
    float height = 0.0F;
    float minWidth = 1.0F;
    float minHeight = 1.0F;
    float spacing = 0.0F;
};

// Resolves bounded retained widget geometry against an explicit software
// surface. It has no draw, input dispatch, Farm, or world authority.
class UiLayoutResolver {
public:
    static constexpr uint16_t kMaxSpecs = UiInputRouter::kMaxWidgets;

    // Specs are evaluated in deterministic caller order. Each child widget's
    // parent must already have a resolved spec. Router geometry commits only
    // after every spec has validated on a candidate router copy.
    bool Apply(const UiLayoutSurface& surface, const std::vector<UiLayoutSpec>& specs, UiInputRouter& router);
    [[nodiscard]] UiLayoutError LastError() const { return lastError_; }

private:
    UiLayoutError lastError_ = UiLayoutError::None;
};

} // namespace NeoEngine
