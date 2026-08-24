#include "BehaviorTree.h"

void BehaviorTree::AddNode(const BTNode& node)
{
    nodes.push_back(node);
}

BTStatus BehaviorTree::Tick()
{
    for(auto& n : nodes)
    {
        BTStatus s = n.tick();

        if(s != BTStatus::Success)
            return s;
    }

    return BTStatus::Success;
}
