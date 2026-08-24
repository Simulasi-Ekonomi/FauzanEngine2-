#pragma once
#include <string>
#include <vector>
#include <functional>

namespace NeoEngine {

struct UserWidget {
    std::string name;
    std::string type; // "button", "label", "image", "slider", "checkbox"
    float x, y, width, height;
    std::string text;
    float value = 0;
    bool visible = true;
    bool interactable = true;
    std::function<void()> onPress;
    std::function<void(float)> onValueChanged;
};

class UserWidgetCore {
public:
    UserWidgetCore() = default;

    UserWidget* CreateWidget(const std::string& name, const std::string& type,
                             float x, float y, float w, float h) {
        UserWidget w;
        w.name = name;
        w.type = type;
        w.x = x; w.y = y; w.width = w; w.height = h;
        m_Widgets.push_back(w);
        return &m_Widgets.back();
    }

    UserWidget* GetWidget(const std::string& name) {
        for (auto& w : m_Widgets) {
            if (w.name == name) return &w;
        }
        return nullptr;
    }

    void RemoveWidget(const std::string& name) {
        m_Widgets.erase(std::remove_if(m_Widgets.begin(), m_Widgets.end(),
            [&](const UserWidget& w) { return w.name == name; }), m_Widgets.end());
    }

    void SetText(const std::string& name, const std::string& text) {
        auto* w = GetWidget(name);
        if (w) w->text = text;
    }

    void SetValue(const std::string& name, float value) {
        auto* w = GetWidget(name);
        if (w) {
            w->value = value;
            if (w->onValueChanged) w->onValueChanged(value);
        }
    }

    void ProcessInput(float mx, float my, bool pressed) {
        for (auto& w : m_Widgets) {
            if (!w.visible || !w.interactable) continue;
            bool inside = (mx >= w.x && mx <= w.x + w.width &&
                          my >= w.y && my <= w.y + w.height);
            if (inside && pressed && w.onPress) w.onPress();
        }
    }

    const std::vector<UserWidget>& GetWidgets() const { return m_Widgets; }
    void Clear() { m_Widgets.clear(); }

private:
    std::vector<UserWidget> m_Widgets;
};

} // namespace NeoEngine
