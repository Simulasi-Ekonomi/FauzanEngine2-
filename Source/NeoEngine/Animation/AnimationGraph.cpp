#include "AnimationGraph.h"

void AnimationGraph::AddNode(const AnimationNode& node)
{
    nodes.push_back(node);
}

void AnimationGraph::Update()
{
    for(auto& n : nodes)
    {
        if(n.update)
            n.update();
    }
}
