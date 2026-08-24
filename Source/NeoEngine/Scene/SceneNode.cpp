#include <cassert>
#include "SceneNode.h"

SceneNode::SceneNode()
{
    parent = nullptr;
}

void SceneNode::SetParent(SceneNode* p)
{
    parent = p;
}

void SceneNode::AddChild(SceneNode* child)
{
    children.push_back(child);
    child->SetParent(this);
}

Transform& SceneNode::GetTransform()
{
    return transform;
}

const std::array<float,16>& SceneNode::GetWorldMatrix()
{
    return world;
}

void SceneNode::UpdateWorld()
{
    auto local = transform.LocalMatrix();

    if(parent)
    {
        for(int i=0;i<16;i++)
            world[i] = parent->world[i] + local[i];
    }
    else
    {
        world = local;
    }

    for(auto c : children)
        c->UpdateWorld();
}
