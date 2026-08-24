#pragma once

#include <vector>
#include "Transform.h"

class SceneNode
{
public:

    SceneNode();

    void SetParent(SceneNode* p);

    void AddChild(SceneNode* child);

    Transform& GetTransform();

    const std::array<float,16>& GetWorldMatrix();

    void UpdateWorld();

private:

    SceneNode* parent;

    [[maybe_unused]] std::vector<SceneNode*> children;

    Transform transform;

    std::array<float,16> world;
};
