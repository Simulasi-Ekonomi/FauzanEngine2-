#pragma once

#include <cstdint>
#include <vector>

namespace NeoEngine {
class UiLayoutResolver;
enum class UiError : uint8_t { None, InvalidWidget, DuplicateWidget, MissingParent, Capacity, InvalidRect, FocusUnavailable, InvalidKey };
enum class UiPointerPhase : uint8_t { Press, Move, Release };
enum class UiKeyboardKey : uint8_t { TabForward, TabBackward, Activate, ClearFocus };
struct UiRect { float x = 0.0F; float y = 0.0F; float width = 0.0F; float height = 0.0F; };
struct UiWidgetSpec { uint16_t id = 0; uint16_t parentId = 0; UiRect rect{}; int16_t zOrder = 0; bool interactive = false; bool visible = true; bool focusable = false; };
struct UiPointerResult { uint16_t targetId = 0; bool captured = false; bool consumed = false; };
struct UiKeyboardResult { uint16_t targetId = 0; bool consumed = false; bool activated = false; };
class UiInputRouter {
public:
    static constexpr uint16_t kMaxWidgets = 256;
    bool AddWidget(const UiWidgetSpec& spec);
    bool RemoveWidget(uint16_t id);
    UiPointerResult RoutePointer(float x, float y, UiPointerPhase phase);
    UiKeyboardResult RouteKeyboard(UiKeyboardKey key);
    bool SetFocus(uint16_t id);
    [[nodiscard]] uint16_t HitTest(float x, float y) const;
    [[nodiscard]] std::vector<UiWidgetSpec> RenderableWidgets() const;
    [[nodiscard]] uint16_t CapturedWidget() const { return capturedWidget_; }
    [[nodiscard]] uint16_t FocusedWidget() const { return focusedWidget_; }
    [[nodiscard]] UiError LastError() const { return lastError_; }
private:
    friend class UiLayoutResolver;
    struct Widget { UiWidgetSpec spec{}; uint16_t sequence = 0; };
    [[nodiscard]] bool HasWidget(uint16_t id) const;
    [[nodiscard]] bool IsFocusable(uint16_t id) const;
    std::vector<Widget> widgets_;
    uint16_t sequence_ = 0;
    uint16_t capturedWidget_ = 0;
    uint16_t focusedWidget_ = 0;
    UiError lastError_ = UiError::None;
};
} // namespace NeoEngine
