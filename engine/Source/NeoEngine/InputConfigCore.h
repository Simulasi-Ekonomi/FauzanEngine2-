#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace NeoEngine {

struct InputAction {
    std::string name;
    int keyCode;
    bool pressed = false;
    bool justPressed = false;
    bool justReleased = false;
};

class InputConfigCore {
public:
    void BindAction(const std::string& action, int keyCode) {
        m_Actions[action] = {action, keyCode, false, false, false};
    }

    void ProcessKeyEvent(int keyCode, bool pressed) {
        for (auto& [name, action] : m_Actions) {
            if (action.keyCode == keyCode) {
                if (pressed && !action.pressed) action.justPressed = true;
                else if (!pressed && action.pressed) action.justReleased = true;
                action.pressed = pressed;
                return;
            }
        }
    }

    void ClearFrameFlags() {
        for (auto& [name, action] : m_Actions) {
            action.justPressed = false;
            action.justReleased = false;
        }
    }

    bool IsPressed(const std::string& action) const {
        auto it = m_Actions.find(action);
        return it != m_Actions.end() && it->second.pressed;
    }

    bool JustPressed(const std::string& action) const {
        auto it = m_Actions.find(action);
        return it != m_Actions.end() && it->second.justPressed;
    }

    bool JustReleased(const std::string& action) const {
        auto it = m_Actions.find(action);
        return it != m_Actions.end() && it->second.justReleased;
    }

    void SetupDefaultBindings() {
        BindAction("MoveForward", 87);   // W
        BindAction("MoveBackward", 83);  // S
        BindAction("MoveLeft", 65);      // A
        BindAction("MoveRight", 68);     // D
        BindAction("Jump", 32);          // Space
        BindAction("Attack", 0);         // Touch
        BindAction("Interact", 69);      // E
    }

private:
    std::unordered_map<std::string, InputAction> m_Actions;
};

} // namespace NeoEngine
