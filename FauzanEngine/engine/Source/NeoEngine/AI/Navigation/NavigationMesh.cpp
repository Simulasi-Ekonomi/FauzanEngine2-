#include "NavigationMesh.h"

void NavigationMesh::AddNode(const NavNode& node)
{
    nodes.push_back(node);
}

const std::vector<NavNode>& NavigationMesh::Nodes() const
{
    return nodes;
}
