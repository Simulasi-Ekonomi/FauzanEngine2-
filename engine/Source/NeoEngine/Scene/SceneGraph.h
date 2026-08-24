#pragma once

#include <vector>
#include "SceneNode.h"

class SceneGraph
{
public:

    SceneNode* CreateNode();

    void Update();

private:

    [[maybe_unused]] std::vector<SceneNode*> nodes;
};
