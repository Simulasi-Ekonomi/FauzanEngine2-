#include "AnimationGraph.h"
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
bool AnimationGraph::AddNode(const AnimationNode& node) {
    if (!node.update || nodes.size() >= kMaxNodes || updating_ || revision_ == std::numeric_limits<uint64_t>::max() || nodes.capacity() > kMaxNodes) return false;
    const std::size_t sizeBefore = nodes.size();
    try {
        nodes.push_back(node);
    } catch (...) {
        return false;
    }
    if (nodes.size() != sizeBefore + 1U || nodes.size() > kMaxNodes) {
        if (nodes.size() > sizeBefore) nodes.pop_back();
        return false;
    }
    ++revision_;
    if (revision_ == 0U) {
        nodes.pop_back();
        return false;
    }
    return true;
}
bool AnimationGraph::RemoveNode(std::size_t index) {
    if (updating_ || index >= nodes.size() || revision_ == std::numeric_limits<uint64_t>::max() || nodes.capacity() > kMaxNodes) return false;
    const std::size_t sizeBefore = nodes.size();
    try {
        nodes.erase(nodes.begin() + static_cast<std::ptrdiff_t>(index));
    } catch (...) {
        return false;
    }
    if (nodes.size() + 1U != sizeBefore) return false;
    ++revision_;
    if (revision_ == 0U) return false;
    return true;
}
void AnimationGraph::Clear() noexcept {
    if (updating_) return;
    if (!nodes.empty()) { if (revision_ == std::numeric_limits<uint64_t>::max()) return; nodes.clear(); ++revision_; }
}
bool AnimationGraph::Update() {
    if (updating_) return false;
    if (nodes.size() > kMaxNodes || nodes.size() > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())) return false;
    if (nodes.empty()) return true;
    if (nodes.size() > kMaxNodes || revision_ == std::numeric_limits<uint64_t>::max()) return false;
    updating_ = true;
    const std::size_t count = nodes.size();
    const uint64_t revisionBeforeUpdate = revision_;
    try {
        for (std::size_t i = 0; i < count; ++i) {
            if (!nodes[i].update) {
                updating_ = false;
                return false;
            }
            nodes[i].update();
            if (nodes.size() != count || nodes.capacity() > kMaxNodes || revision_ != revisionBeforeUpdate) {
                updating_ = false;
                return false;
            }
        }
    } catch (...) {
        updating_ = false;
        return false;
    }
    updating_ = false;
    if (nodes.size() != count) return false;
    return revision_ == revisionBeforeUpdate;
}
