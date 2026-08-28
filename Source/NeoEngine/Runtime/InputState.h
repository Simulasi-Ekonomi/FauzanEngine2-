#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace NeoEngine {

enum class InputError : uint8_t { None, InvalidAction, DuplicateAction, MissingAction, Capacity, QueueFull, InvalidMetadata };
enum class InputDeviceType : uint8_t { Keyboard = 1, Mouse = 2, Touch = 3, Gamepad = 4 };

constexpr int32_t MakeInputCode(InputDeviceType device, uint16_t control) {
    return static_cast<int32_t>((static_cast<uint32_t>(device) << 16U) | control);
}

struct InputSnapshot {
    bool pressed = false;
    bool justPressed = false;
    bool justReleased = false;
};

struct InputPointerSnapshot {
    bool active = false;
    uint32_t pointerId = 0U;
    float normalizedX = 0.0F;
    float normalizedY = 0.0F;
};

struct InputControllerSnapshot {
    bool connected = false;
    float leftAxisX = 0.0F;
    float leftAxisY = 0.0F;
};

struct InputFrameMetadata {
    bool quitRequested = false;
    bool focusLost = false;
    InputPointerSnapshot pointer{};
    InputControllerSnapshot controller{};
};

struct InputStateSummary {
    uint16_t boundActions = 0U;
    uint16_t pressedActions = 0U;
    uint16_t justPressedActions = 0U;
    uint16_t justReleasedActions = 0U;
    uint16_t pendingEvents = 0U;
};

class InputState {
public:
    static constexpr size_t kMaxActions = 128;
    static constexpr size_t kMaxEvents = 512;

    bool Bind(std::string action, int32_t code);
    bool Rebind(const std::string& action, int32_t code);
    bool Push(int32_t code, bool pressed);
    void BeginFrame();
    void ReleaseAll();
    void ClearFrameMetadata();
    void MarkQuitRequested() { metadata_.quitRequested = true; }
    void MarkFocusLost() { metadata_.focusLost = true; }
    bool SetTouchPointer(uint32_t pointerId, float normalizedX, float normalizedY, bool active);
    bool SetControllerConnected(bool connected);
    bool SetControllerAxis(uint8_t axis, float value);

    [[nodiscard]] InputSnapshot Query(const std::string& action) const;
    [[nodiscard]] bool HasAction(const std::string& action) const;
    [[nodiscard]] InputStateSummary Summary() const;
    [[nodiscard]] InputFrameMetadata FrameMetadata() const { return metadata_; }
    [[nodiscard]] InputError LastError() const { return error_; }

private:
    struct Action {
        std::string id;
        int32_t code = 0;
        InputSnapshot state{};
        bool releasePending = false;
    };
    struct Event {
        int32_t code = 0;
        bool pressed = false;
    };

    std::vector<Action> actions_;
    std::vector<Event> events_;
    InputFrameMetadata metadata_{};
    InputError error_ = InputError::None;
};

} // namespace NeoEngine
