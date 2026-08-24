#pragma once
#include <string>
#include <vector>
#include <functional>

namespace NeoEngine {

struct UserWidget {
    std::string name;
    std::string type;
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
        UserWidget widget;
        widget.name = name;
        widget.type = type;
        widget.x = x; widget.y = y; widget.width = w; widget.height = h;
        m_Widgets.push_back(widget);
        return &m_Widgets.back();
    }

    UserWidget* GetWidget(const std::string& name) {
        for (auto& widget : m_Widgets) {
            if (widget.name == name) return &widget;
        }
        return nullptr;
    }

    void RemoveWidget(const std::string& name) {
        m_Widgets.erase(std::remove_if(m_Widgets.begin(), m_Widgets.end(),
            [&](const UserWidget& w) { return w.name == name; }), m_Widgets.end());
    }

    const std::vector<UserWidget>& GetWidgets() const { return m_Widgets; }
    void Clear() { m_Widgets.clear(); }

private:
    std::vector<UserWidget> m_Widgets;
};

} // namespace NeoEngine
