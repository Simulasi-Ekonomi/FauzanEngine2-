#pragma once
#include <vector>
#include <functional>

enum class BTStatus
{
    Success,
    Failure,
    Running
};

struct BTNode
{
    std::function<BTStatus()> tick;
};

class BehaviorTree
{
public:

    void AddNode(const BTNode& node);

    BTStatus Tick();

private:

    std::vector<BTNode> nodes;
};
