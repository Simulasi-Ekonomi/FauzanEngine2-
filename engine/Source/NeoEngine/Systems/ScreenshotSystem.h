#pragma once
#include <string>
#include <vector>
#include <functional>
#include <chrono>

namespace NeoEngine {

struct Screenshot {
    std::string id, playerId;
    std::string filePath;
    float posX, posY, posZ;
    std::string caption;
    bool shared = false;
    int likes = 0;
    std::chrono::system_clock::time_point takenAt;
};

class ScreenshotSystem {
private:
    std::vector<Screenshot> m_Screenshots;
    std::function<void(const Screenshot&)> m_OnScreenshotTaken;
    std::function<void(const Screenshot&)> m_OnShared;

public:
    Screenshot* TakeScreenshot(const std::string& playerId, const std::string& path,
                               float x, float y, float z, const std::string& caption = "") {
        m_Screenshots.push_back({std::to_string(m_Screenshots.size()), playerId, path, x, y, z, caption, false, 0,
                                std::chrono::system_clock::now()});
        if (m_OnScreenshotTaken) m_OnScreenshotTaken(m_Screenshots.back());
        return &m_Screenshots.back();
    }
    bool ShareScreenshot(const std::string& id) {
        for (auto& s : m_Screenshots) if (s.id == id) { s.shared = true; if (m_OnShared) m_OnShared(s); return true; }
        return false;
    }
    bool LikeScreenshot(const std::string& id) {
        for (auto& s : m_Screenshots) if (s.id == id) { s.likes++; return true; }
        return false;
    }
    std::vector<Screenshot> GetPlayerScreenshots(const std::string& playerId) const {
        std::vector<Screenshot> result;
        for (auto& s : m_Screenshots) if (s.playerId == playerId) result.push_back(s);
        return result;
    }
    std::vector<Screenshot> GetSharedScreenshots() const {
        std::vector<Screenshot> result;
        for (auto& s : m_Screenshots) if (s.shared) result.push_back(s);
        return result;
    }
    void SetOnScreenshotTaken(std::function<void(const Screenshot&)> cb) { m_OnScreenshotTaken = cb; }
    void SetOnShared(std::function<void(const Screenshot&)> cb) { m_OnShared = cb; }
};

} // namespace NeoEngine
