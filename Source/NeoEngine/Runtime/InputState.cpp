#include "InputState.h"

#include <cmath>
#include <utility>

namespace NeoEngine {

bool InputState::Bind(std::string id, int32_t code) {
    if (id.empty() || id.size() > 64U) {
        error_ = InputError::InvalidAction;
        return false;
    }
    for (const auto& action : actions_) {
        if (action.id == id) {
            error_ = InputError::DuplicateAction;
            return false;
        }
    }
    if (actions_.size() >= kMaxActions) {
        error_ = InputError::Capacity;
        return false;
    }
    actions_.push_back({std::move(id), code, {}});
    error_ = InputError::None;
    return true;
}

bool InputState::Rebind(const std::string& id, int32_t code) {
    for (auto& action : actions_) {
        if (action.id == id) {
            action.code = code;
            action.state = {};
            error_ = InputError::None;
            return true;
        }
    }
    error_ = InputError::MissingAction;
    return false;
}

bool InputState::Push(int32_t code, bool pressed) {
    if (events_.size() >= kMaxEvents) {
        error_ = InputError::QueueFull;
        return false;
    }
    events_.push_back({code, pressed});
    error_ = InputError::None;
    return true;
}

void InputState::BeginFrame() {
    for (auto& action : actions_) {
        action.state.justPressed = false;
        action.state.justReleased = action.releasePending;
        action.releasePending = false;
    }
    for (const auto& event : events_) {
        for (auto& action : actions_) {
            if (action.code != event.code) continue;
            if (event.pressed && !action.state.pressed) action.state.justPressed = true;
            if (!event.pressed && action.state.pressed) action.state.justReleased = true;
            action.state.pressed = event.pressed;
        }
    }
    events_.clear();
    error_ = InputError::None;
}

void InputState::ReleaseAll() {
    for (auto& action : actions_) {
        if (action.state.pressed) action.releasePending = true;
        action.state.pressed = false;
    }
    events_.clear();
    error_ = InputError::None;
}

void InputState::ClearFrameMetadata() {
    metadata_ = {};
    error_ = InputError::None;
}

bool InputState::SetTouchPointer(uint32_t pointerId, float normalizedX, float normalizedY, bool active) {
    if (!std::isfinite(normalizedX) || !std::isfinite(normalizedY) || normalizedX < 0.0F || normalizedX > 1.0F || normalizedY < 0.0F || normalizedY > 1.0F) {
        error_ = InputError::InvalidMetadata;
        return false;
    }
    metadata_.pointer = {active, pointerId, normalizedX, normalizedY};
    error_ = InputError::None;
    return true;
}

bool InputState::SetControllerConnected(bool connected) {
    metadata_.controller.connected = connected;
    if (!connected) {
        metadata_.controller.leftAxisX = 0.0F;
        metadata_.controller.leftAxisY = 0.0F;
    }
    error_ = InputError::None;
    return true;
}

bool InputState::SetControllerAxis(uint8_t axis, float value) {
    if ((axis != 0U && axis != 1U) || !std::isfinite(value) || value < -1.0F || value > 1.0F) {
        error_ = InputError::InvalidMetadata;
        return false;
    }
    metadata_.controller.connected = true;
    if (axis == 0U) metadata_.controller.leftAxisX = value;
    else metadata_.controller.leftAxisY = value;
    error_ = InputError::None;
    return true;
}

InputSnapshot InputState::Query(const std::string& id) const {
    for (const auto& action : actions_) {
        if (action.id == id) return action.state;
    }
    return {};
}

bool InputState::HasAction(const std::string& id) const {
    for (const auto& action : actions_) {
        if (action.id == id) return true;
    }
    return false;
}

InputStateSummary InputState::Summary() const {
    InputStateSummary summary{};
    summary.boundActions = static_cast<uint16_t>(actions_.size());
    summary.pendingEvents = static_cast<uint16_t>(events_.size());
    for (const auto& action : actions_) {
        if (action.state.pressed) ++summary.pressedActions;
        if (action.state.justPressed) ++summary.justPressedActions;
        if (action.state.justReleased) ++summary.justReleasedActions;
    }
    return summary;
}

} // namespace NeoEngine
