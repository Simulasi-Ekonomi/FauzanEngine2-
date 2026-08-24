#include <cassert>
#include "SceneGraph.h"

SceneNode* SceneGraph::CreateNode()
{
    SceneNode* node = new SceneNode();
    nodes.push_back(node);
    return node;
}

void SceneGraph::Update()
{
    for(auto n : nodes)
    {
        if(n->GetWorldMatrix()[15] == 0)
            n->UpdateWorld();
    }
}
