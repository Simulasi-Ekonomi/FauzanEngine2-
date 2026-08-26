#include "UiLayoutResolver.h"

#include <algorithm>
#include <cmath>

namespace NeoEngine {
namespace {

bool IsFinitePositive(float value) { return std::isfinite(value) && value > 0.0F; }
bool IsFiniteNonNegative(float value) { return std::isfinite(value) && value >= 0.0F; }
bool Contains(const std::vector<uint16_t>& values, uint16_t value) { return std::find(values.begin(), values.end(), value) != values.end(); }

struct VerticalCursor { uint16_t parentId = 0U; float nextY = 0.0F; };

VerticalCursor* FindCursor(std::vector<VerticalCursor>& cursors, uint16_t parentId) {
    const auto found = std::find_if(cursors.begin(), cursors.end(), [parentId](const VerticalCursor& cursor) { return cursor.parentId == parentId; });
    return found == cursors.end() ? nullptr : &*found;
}

bool ValidPadding(const UiLayoutPadding& padding) {
    return IsFiniteNonNegative(padding.left) && IsFiniteNonNegative(padding.top) && IsFiniteNonNegative(padding.right) && IsFiniteNonNegative(padding.bottom);
}

} // namespace

bool UiLayoutResolver::Apply(const UiLayoutSurface& surface, const std::vector<UiLayoutSpec>& specs, UiInputRouter& router) {
    if (!IsFinitePositive(surface.width) || !IsFinitePositive(surface.height)) { lastError_ = UiLayoutError::InvalidSurface; return false; }
    if (specs.empty()) { lastError_ = UiLayoutError::EmptyPlan; return false; }
    if (specs.size() > kMaxSpecs) { lastError_ = UiLayoutError::Capacity; return false; }

    UiInputRouter candidate = router;
    const auto findWidget = [&candidate](uint16_t id) -> UiInputRouter::Widget* {
        const auto found = std::find_if(candidate.widgets_.begin(), candidate.widgets_.end(), [id](const UiInputRouter::Widget& widget) { return widget.spec.id == id; });
        return found == candidate.widgets_.end() ? nullptr : &*found;
    };
    std::vector<uint16_t> resolvedIds;
    std::vector<VerticalCursor> cursors;
    resolvedIds.reserve(specs.size());
    cursors.reserve(specs.size());

    for (const UiLayoutSpec& spec : specs) {
        if (spec.widgetId == 0U || Contains(resolvedIds, spec.widgetId)) { lastError_ = UiLayoutError::DuplicateWidget; return false; }
        if (!ValidPadding(spec.padding) || !IsFiniteNonNegative(spec.spacing) || !IsFiniteNonNegative(spec.width) || !IsFiniteNonNegative(spec.height) || !IsFinitePositive(spec.minWidth) || !IsFinitePositive(spec.minHeight)) { lastError_ = UiLayoutError::InvalidSpec; return false; }

        UiInputRouter::Widget* widget = findWidget(spec.widgetId);
        if (widget == nullptr) { lastError_ = UiLayoutError::MissingWidget; return false; }
        const uint16_t parentId = widget->spec.parentId;
        UiRect container{0.0F, 0.0F, surface.width, surface.height};
        if (parentId != 0U) {
            if (!Contains(resolvedIds, parentId)) { lastError_ = UiLayoutError::ParentUnresolved; return false; }
            UiInputRouter::Widget* parent = findWidget(parentId);
            if (parent == nullptr) { lastError_ = UiLayoutError::ParentUnresolved; return false; }
            container = parent->spec.rect;
        }
        const float contentWidth = container.width - spec.padding.left - spec.padding.right;
        const float contentHeight = container.height - spec.padding.top - spec.padding.bottom;
        if (!IsFinitePositive(contentWidth) || !IsFinitePositive(contentHeight)) { lastError_ = UiLayoutError::InsufficientSpace; return false; }

        float width = spec.width;
        float height = spec.height;
        if (spec.flow == UiLayoutFlow::VerticalStack) {
            if (!IsFinitePositive(height)) { lastError_ = UiLayoutError::InvalidSpec; return false; }
            if (width == 0.0F) width = contentWidth;
        } else if (spec.anchor == UiLayoutAnchor::Fill) {
            if (width == 0.0F) width = contentWidth;
            if (height == 0.0F) height = contentHeight;
        }
        if (!IsFinitePositive(width) || !IsFinitePositive(height) || width < spec.minWidth || height < spec.minHeight || width > contentWidth || height > contentHeight) { lastError_ = UiLayoutError::InsufficientSpace; return false; }

        UiRect rect{};
        if (spec.flow == UiLayoutFlow::VerticalStack) {
            VerticalCursor* cursor = FindCursor(cursors, parentId);
            if (cursor == nullptr) { cursors.push_back({parentId, container.y}); cursor = &cursors.back(); }
            rect = {container.x + spec.padding.left, cursor->nextY + spec.padding.top, width, height};
            cursor->nextY = rect.y + rect.height + spec.padding.bottom + spec.spacing;
        } else {
            switch (spec.anchor) {
                case UiLayoutAnchor::TopLeft: rect = {container.x + spec.padding.left, container.y + spec.padding.top, width, height}; break;
                case UiLayoutAnchor::TopRight: rect = {container.x + container.width - spec.padding.right - width, container.y + spec.padding.top, width, height}; break;
                case UiLayoutAnchor::BottomLeft: rect = {container.x + spec.padding.left, container.y + container.height - spec.padding.bottom - height, width, height}; break;
                case UiLayoutAnchor::BottomRight: rect = {container.x + container.width - spec.padding.right - width, container.y + container.height - spec.padding.bottom - height, width, height}; break;
                case UiLayoutAnchor::Fill: rect = {container.x + spec.padding.left, container.y + spec.padding.top, width, height}; break;
            }
        }
        if (!std::isfinite(rect.x) || !std::isfinite(rect.y) || rect.x < container.x || rect.y < container.y || rect.x + rect.width > container.x + container.width || rect.y + rect.height > container.y + container.height) { lastError_ = UiLayoutError::InsufficientSpace; return false; }
        widget->spec.rect = rect;
        resolvedIds.push_back(spec.widgetId);
    }

    router = std::move(candidate);
    lastError_ = UiLayoutError::None;
    return true;
}

} // namespace NeoEngine
