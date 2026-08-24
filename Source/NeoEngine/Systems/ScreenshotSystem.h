#pragma once
#include <string>
#include <vector>
#include <chrono>

namespace NeoEngine {

struct Screenshot {
    std::string id, playerId, filePath;
    std::chrono::system_clock::time_point takenAt;
};

class ScreenshotSystem {
private:
    std::vector<Screenshot> m_Screenshots;

public:
    void TakeScreenshot(const std::string& playerId, const std::string& path) {
        m_Screenshots.push_back({std::to_string(m_Screenshots.size()), playerId, path, std::chrono::system_clock::now()});
    }
};

} // namespace NeoEngine
