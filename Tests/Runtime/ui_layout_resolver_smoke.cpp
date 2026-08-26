#include "Runtime/SoftwareRenderer.h"
#include "Runtime/UiCanvasRenderer.h"
#include "Runtime/UiLayoutResolver.h"

#include <cmath>
#include <cstdio>
#include <vector>

namespace {
const NeoEngine::UiWidgetSpec* Find(const std::vector<NeoEngine::UiWidgetSpec>& widgets, uint16_t id) {
    for (const NeoEngine::UiWidgetSpec& widget : widgets) if (widget.id == id) return &widget;
    return nullptr;
}
bool Near(float left, float right) { return std::fabs(left - right) < 0.001F; }
}

int main() {
    using namespace NeoEngine;
    UiInputRouter router;
    if (!router.AddWidget({1U, 0U, {0, 0, 1, 1}, 0, false, true, false}) || !router.AddWidget({2U, 1U, {0, 0, 1, 1}, 1, true, true, true}) || !router.AddWidget({3U, 1U, {0, 0, 1, 1}, 2, true, true, true}) || !router.AddWidget({4U, 1U, {0, 0, 1, 1}, 3, true, true, true}) || !router.AddWidget({5U, 1U, {0, 0, 1, 1}, 4, true, true, true})) return 2;
    const std::vector<UiWidgetSpec> before = router.RenderableWidgets();
    UiLayoutResolver layout;
    const std::vector<UiLayoutSpec> plan{
        {1U, UiLayoutAnchor::TopRight, UiLayoutFlow::None, {2, 2, 2, 2}, 58, 80, 58, 80, 0},
        {2U, UiLayoutAnchor::TopLeft, UiLayoutFlow::VerticalStack, {4, 4, 4, 2}, 0, 10, 20, 10, 2},
        {3U, UiLayoutAnchor::TopLeft, UiLayoutFlow::VerticalStack, {4, 4, 4, 2}, 0, 10, 20, 10, 2},
        {4U, UiLayoutAnchor::TopLeft, UiLayoutFlow::VerticalStack, {4, 4, 4, 2}, 0, 10, 20, 10, 2},
        {5U, UiLayoutAnchor::TopLeft, UiLayoutFlow::VerticalStack, {4, 4, 4, 2}, 0, 10, 20, 10, 2},
    };
    if (!layout.Apply({128, 96}, plan, router) || layout.LastError() != UiLayoutError::None || router.HitTest(77, 8) != 2U || router.HitTest(77, 26) != 3U || router.HitTest(77, 44) != 4U || router.HitTest(77, 62) != 5U) return 3;
    const std::vector<UiWidgetSpec> resolved = router.RenderableWidgets();
    const UiWidgetSpec* panel = Find(resolved, 1U); const UiWidgetSpec* till = Find(resolved, 2U); const UiWidgetSpec* harvest = Find(resolved, 5U);
    if (panel == nullptr || till == nullptr || harvest == nullptr || !Near(panel->rect.x, 68) || !Near(panel->rect.y, 2) || !Near(panel->rect.width, 58) || !Near(till->rect.x, 72) || !Near(till->rect.y, 6) || !Near(till->rect.width, 50) || !Near(harvest->rect.y, 60)) return 4;
    UiCanvasRenderer canvas;
    if (!canvas.SetStyle({1U, 0xFF202040U}) || !canvas.SetStyle({2U, 0xFF206020U}) || !canvas.SetStyle({3U, 0xFF204060U}) || !canvas.SetStyle({4U, 0xFF206060U}) || !canvas.SetStyle({5U, 0xFF604020U}) || !canvas.SetLabel({2U, "TILL", 2, 2, 1, 0xFFFFFFFFU}) || !canvas.SetLabel({3U, "PLANT", 2, 2, 1, 0xFFFFFFFFU}) || !canvas.SetLabel({4U, "WATER", 2, 2, 1, 0xFFFFFFFFU}) || !canvas.SetLabel({5U, "HARVEST", 2, 2, 1, 0xFFFFFFFFU})) return 5;
    SoftwareRenderer renderer;
    if (!renderer.Initialize(128, 96) || !renderer.Clear(0xFF000000U)) return 6;
    const uint64_t clearHash = renderer.FrameHash();
    if (!canvas.Draw(router, renderer)) { std::fprintf(stderr, "draw=%u\n", static_cast<unsigned>(canvas.LastError())); return 7; }
    if (renderer.FrameHash() == 0U || renderer.FrameHash() == clearHash) return 8;
    const uint64_t frameHash = renderer.FrameHash();
    const std::vector<UiWidgetSpec> preserved = router.RenderableWidgets();
    const std::vector<UiLayoutSpec> invalid{{1U, UiLayoutAnchor::TopRight, UiLayoutFlow::None, {2, 2, 2, 2}, 58, 80, 58, 80, 0}, {2U, UiLayoutAnchor::TopLeft, UiLayoutFlow::VerticalStack, {4, 4, 4, 2}, 0, 10, 20, 10, 2}, {3U, UiLayoutAnchor::TopLeft, UiLayoutFlow::VerticalStack, {4, 4, 4, 2}, 0, 10, 20, 10, 2}, {4U, UiLayoutAnchor::TopLeft, UiLayoutFlow::VerticalStack, {4, 4, 4, 2}, 0, 10, 20, 10, 2}, {5U, UiLayoutAnchor::TopLeft, UiLayoutFlow::VerticalStack, {4, 4, 4, 80}, 0, 10, 20, 10, 2}};
    if (layout.Apply({128, 96}, invalid, router) || layout.LastError() != UiLayoutError::InsufficientSpace || router.RenderableWidgets().size() != preserved.size()) return 9;
    for (size_t index = 0U; index < preserved.size(); ++index) if (!Near(router.RenderableWidgets()[index].rect.x, preserved[index].rect.x) || !Near(router.RenderableWidgets()[index].rect.y, preserved[index].rect.y)) return 10;
    UiInputRouter repeated = router;
    if (!layout.Apply({128, 96}, plan, repeated)) return 11;
    SoftwareRenderer repeatedRenderer;
    if (!repeatedRenderer.Initialize(128, 96) || !repeatedRenderer.Clear(0xFF000000U) || !canvas.Draw(repeated, repeatedRenderer) || repeatedRenderer.FrameHash() != frameHash) return 12;
    if (layout.Apply({0, 96}, plan, router) || layout.LastError() != UiLayoutError::InvalidSurface || router.RenderableWidgets().size() != preserved.size()) return 13;
    std::printf("UI_LAYOUT_RESOLVER_SMOKE_OK panel=1 buttons=4 anchor=1 padding=1 stack=1 click=1 atomic=1 hash=%llu\n", static_cast<unsigned long long>(frameHash));
    return 0;
}
