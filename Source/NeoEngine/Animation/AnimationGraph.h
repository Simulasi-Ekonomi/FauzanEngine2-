#pragma once
#include <vector>
#include <functional>

struct AnimationNode
{
    std::function<void()> update;
};

class AnimationGraph
{
public:

    void AddNode(const AnimationNode& node);
    void Update();

private:

    std::vector<AnimationNode> nodes;
};
