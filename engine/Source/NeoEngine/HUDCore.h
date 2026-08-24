#pragma once
#include <string>
#include <vector>
#include <functional>

namespace NeoEngine {

struct HUDElement {
    std::string id;
    std::string type;
    float x, y;
    std::string text;
    float value = 0;
    float maxValue = 100;
    bool visible = true;
};

class HUDCore {
public:
    HUDCore() = default;

    void AddHealthBar(const std::string& id, float x, float y) {
        HUDElement el{id, "healthbar", x, y, "", 100, 100, true};
        m_Elements.push_back(el);
    }

    void AddScoreDisplay(const std::string& id, float x, float y) {
        HUDElement el{id, "score", x, y, "Score: 0", 0, 0, true};
        m_Elements.push_back(el);
    }

    void AddMinimap(const std::string& id, float x, float y, float size) {
        HUDElement el{id, "minimap", x, y, "", size, size, true};
        m_Elements.push_back(el);
    }

    void AddAmmoDisplay(const std::string& id, float x, float y) {
        HUDElement el{id, "ammo", x, y, "Ammo: ??", 0, 0, true};
        m_Elements.push_back(el);
    }

    void UpdateHealth(const std::string& id, float current, float max) {
        for (auto& el : m_Elements) {
            if (el.id == id) { el.value = current; el.maxValue = max; break; }
        }
    }

    void UpdateScore(const std::string& id, int score) {
        for (auto& el : m_Elements) {
            if (el.id == id) { el.text = "Score: " + std::to_string(score); break; }
        }
    }

    void UpdateAmmo(const std::string& id, int current, int max) {
        for (auto& el : m_Elements) {
            if (el.id == id) { el.text = "Ammo: " + std::to_string(current) + "/" + std::to_string(max); break; }
        }
    }

    void SetVisible(const std::string& id, bool visible) {
        for (auto& el : m_Elements) {
            if (el.id == id) { el.visible = visible; break; }
        }
    }

    void ShowInteractionPrompt(const std::string& text, float x, float y) {
        // Hapus prompt lama dan tambahkan baru
        m_Elements.erase(std::remove_if(m_Elements.begin(), m_Elements.end(),
            [](const HUDElement& el) { return el.id == "interact_prompt"; }), m_Elements.end());
        HUDElement el{"interact_prompt", "text", x, y, text, 0, 0, true};
        m_Elements.push_back(el);
    }

    void HideInteractionPrompt() {
        SetVisible("interact_prompt", false);
    }

    std::string ToJSON() const {
        std::string json = "{\"hud\":[";
        bool first = true;
        for (const auto& el : m_Elements) {
            if (!first) json += ",";
            json += "{\"id\":\"" + el.id + "\",\"type\":\"" + el.type +
                    "\",\"x\":" + std::to_string(el.x) + ",\"y\":" + std::to_string(el.y) +
                    ",\"text\":\"" + el.text + "\",\"value\":" + std::to_string(el.value) +
                    ",\"max\":" + std::to_string(el.maxValue) + "}";
            first = false;
        }
        json += "]}";
        return json;
    }

    const std::vector<HUDElement>& GetElements() const { return m_Elements; }
    void Clear() { m_Elements.clear(); }

private:
    std::vector<HUDElement> m_Elements;
};

} // namespace NeoEngine
