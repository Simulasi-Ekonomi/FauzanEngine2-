#pragma once
#include <string>
#include <functional>

namespace NeoEngine {

struct StreamRequest {
    std::string assetPath;
    int priority = 0;
    std::function<void(void*)> callback;
    bool operator<(const StreamRequest& other) const {
        return priority < other.priority;
    }
};

} // namespace NeoEngine
