#include "Scene/SceneGraph.h"
#include "Scene/SceneNode.h"
namespace NeoEngine {
void SceneGraph::AddNode(SceneNode* node) { nodes.push_back(node); }
void SceneGraph::RemoveNode(SceneNode* node) {
    auto it = std::find(nodes.begin(), nodes.end(), node);
    if (it != nodes.end()) nodes.erase(it);
}
}
