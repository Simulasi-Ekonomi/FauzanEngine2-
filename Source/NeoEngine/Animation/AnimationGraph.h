#pragma once
#include <vector>
#include <functional>
#include <cstddef>
#include <cstdint>

struct AnimationNode
{
    std::function<void()> update;
};

class AnimationGraph
{
public:
    static constexpr std::size_t kMaxNodes = 4096U;
    bool AddNode(const AnimationNode& node);
    bool RemoveNode(std::size_t index);
    void Clear() noexcept;
    bool Update();
    [[nodiscard]] std::size_t Size() const noexcept { return nodes.size(); }
    [[nodiscard]] std::uint64_t Revision() const noexcept { return revision_; }
    [[nodiscard]] bool IsUpdating() const noexcept { return updating_; }
private:
    std::vector<AnimationNode> nodes;
    std::uint64_t revision_ = 0U;
    bool updating_ = false;
};