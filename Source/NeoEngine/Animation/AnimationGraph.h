#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace NeoEngine {

struct AnimationNode {
    std::function<void()> update;
};

class AnimationGraph {
public:
    static constexpr std::size_t kMaxNodes = 4096U;
    [[nodiscard]] bool AddNode(const AnimationNode& node);
    [[nodiscard]] bool RemoveNode(std::size_t index);
    void Clear() noexcept;
    [[nodiscard]] bool Update();
    [[nodiscard]] std::size_t Size() const noexcept { return nodes.size(); }
    [[nodiscard]] std::uint64_t Revision() const noexcept { return revision_; }
    [[nodiscard]] bool IsUpdating() const noexcept { return updating_; }

private:
    std::vector<AnimationNode> nodes;
    std::uint64_t revision_ = 0U;
    bool updating_ = false;
};

}
