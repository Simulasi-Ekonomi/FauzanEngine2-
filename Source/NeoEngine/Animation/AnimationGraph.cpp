#include "AnimationGraph.h"
#include <cstddef>
#include <utility>
namespace NeoEngine {
bool AnimationGraph::AddNode(const AnimationNode& node) {
    if (!node.update || nodes.size() >= kMaxNodes || updating_) return false;
    try {
        nodes.push_back(node);
    } catch (...) {
        return false;
    }
    ++revision_;
    return true;
}
bool AnimationGraph::RemoveNode(std::size_t index) {
    if (updating_ || index >= nodes.size()) return false;
    try {
        nodes.erase(nodes.begin() + static_cast<std::ptrdiff_t>(index));
    } catch (...) {
        return false;
    }
    ++revision_;
    return true;
}
void AnimationGraph::Clear() noexcept {
    if (updating_) return;
    if (!nodes.empty()) { nodes.clear(); ++revision_; }
}
bool AnimationGraph::Update() {
    if (updating_) return false;
    if (nodes.empty()) return true;
    updating_ = true;
    const std::size_t count = nodes.size();
    try {
        for (std::size_t i = 0; i < count; ++i) {
            if (nodes[i].update) nodes[i].update();
        }
    } catch (...) {
        updating_ = false;
        return false;
    }
    updating_ = false;
    return true;
}
} // namespace NeoEngine
