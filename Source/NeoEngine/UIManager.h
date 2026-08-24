#pragma once
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>
#include <android/log.h>

#define LOG_TAG "UIManager"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace NeoEngine {

struct UIElement {
    std::string id;
    std::string type; // "text", "button", "image", "panel", "progressbar"
    float x = 0, y = 0, width = 100, height = 30;
    std::string text;
    std::string color = "#FFFFFF";
    float fontSize = 14;
    bool visible = true;
    bool enabled = true;
    int zOrder = 0;
    std::function<void()> onClick;
};

class UIManager {
public:
    UIManager() = default;

    std::string AddElement(const std::string& type, float x, float y, float w, float h) {
        std::string id = "ui_" + std::to_string(m_NextId++);
        UIElement el;
        el.id = id;
        el.type = type;
        el.x = x; el.y = y; el.width = w; el.height = h;
        m_Elements[id] = el;
        return id;
    }

    void RemoveElement(const std::string& id) { m_Elements.erase(id); }

    UIElement* GetElement(const std::string& id) {
        auto it = m_Elements.find(id);
        return it != m_Elements.end() ? &it->second : nullptr;
    }

    void SetText(const std::string& id, const std::string& text) {
        auto* el = GetElement(id);
        if (el) el->text = text;
    }

    void SetVisible(const std::string& id, bool visible) {
        auto* el = GetElement(id);
        if (el) el->visible = visible;
    }

    void SetOnClick(const std::string& id, std::function<void()> cb) {
        auto* el = GetElement(id);
        if (el) el->onClick = cb;
    }

    void DisplayHUD(int score, int lives, int level, float fps) {
        // Update or create HUD elements
        if (m_Elements.find("hud_score") == m_Elements.end()) {
            AddElement("text", 16, 16, 150, 24);
            m_Elements["hud_score"].id = "hud_score";
            m_Elements["hud_score"].fontSize = 18;
            m_Elements["hud_score"].color = "#FFD700";
        }
        if (m_Elements.find("hud_lives") == m_Elements.end()) {
            AddElement("text", 16, 48, 150, 24);
            m_Elements["hud_lives"].id = "hud_lives";
            m_Elements["hud_lives"].fontSize = 16;
            m_Elements["hud_lives"].color = "#FF4444";
        }
        SetText("hud_score", "Score: " + std::to_string(score));
        SetText("hud_lives", "Lives: " + std::to_string(lives));
    }

    void ProcessClick(float clickX, float clickY) {
        for (auto& [id, el] : m_Elements) {
            if (!el.visible || !el.enabled || !el.onClick) continue;
            if (clickX >= el.x && clickX <= el.x + el.width &&
                clickY >= el.y && clickY <= el.y + el.height) {
                el.onClick();
                break;
            }
        }
    }

    std::string ToJSON() const {
        std::string json = "{\"elements\":[";
        bool first = true;
        for (const auto& [id, el] : m_Elements) {
            if (!first) json += ",";
            json += "{\"id\":\"" + el.id + "\",\"type\":\"" + el.type +
                    "\",\"x\":" + std::to_string(el.x) + ",\"y\":" + std::to_string(el.y) +
                    ",\"text\":\"" + el.text + "\",\"visible\":" + (el.visible ? "true" : "false") + "}";
            first = false;
        }
        json += "]}";
        return json;
    }

    void Clear() { m_Elements.clear(); }
    size_t GetElementCount() const { return m_Elements.size(); }

private:
    std::unordered_map<std::string, UIElement> m_Elements;
    int m_NextId = 1;
};

} // namespace NeoEngine
