#include "AnimationGraph.h"
#include <cstddef>
#include <limits>
#include <utility>
namespace NeoEngine {
bool AnimationGraph::AddNode(const AnimationNode& node) {
    if (!node.update || nodes.size() >= kMaxNodes || updating_ || revision_ == std::numeric_limits<uint64_t>::max()) return false;
    try {
        nodes.push_back(node);
    } catch (...) {
        return false;
    }
    ++revision_;
    return true;
}
bool AnimationGraph::RemoveNode(std::size_t index) {
    if (updating_ || index >= nodes.size() || revision_ == std::numeric_limits<uint64_t>::max()) return false;
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
    if (!nodes.empty()) { if (revision_ == std::numeric_limits<uint64_t>::max()) return; nodes.clear(); ++revision_; }
}
bool AnimationGraph::Update() {
    if (updating_) return false;
    if (nodes.empty()) return true;
    if (nodes.size() > kMaxNodes || revision_ == std::numeric_limits<uint64_t>::max()) return false;
    updating_ = true;
    const std::size_t count = nodes.size();
    const uint64_t revisionBeforeUpdate = revision_;
    try {
        for (std::size_t i = 0; i < count; ++i) {
            if (nodes[i].update) nodes[i].update();
        }
    } catch (...) {
        updating_ = false;
        return false;
    }
    updating_ = false;
    return revision_ == revisionBeforeUpdate;
}
} // namespace NeoEngine
