#pragma once
#include <vector>

struct NavNode
{
    float x;
    float y;
    float z;
};

class NavigationMesh
{
public:

    void AddNode(const NavNode& node);
    const std::vector<NavNode>& Nodes() const;

private:

    std::vector<NavNode> nodes;
};
