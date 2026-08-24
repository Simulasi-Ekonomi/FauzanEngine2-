#include "SceneGraph.h"
#include <cassert>

SceneGraph::SceneGraph() = default;

SceneGraph::~SceneGraph() {
    for (auto* node : nodes) delete node;
    nodes.clear();
}

SceneNode* SceneGraph::CreateNode() {
    SceneNode* node = new SceneNode();
    nodes.push_back(node);
    return node;
}

void SceneGraph::Update() {
    for (auto* n : nodes) {
        if (n->GetWorldMatrix()[15] == 0)
            n->UpdateWorld();
    }
}

void SceneGraph::RemoveNode(SceneNode* node) {
    auto it = std::find(nodes.begin(), nodes.end(), node);
    if (it != nodes.end()) {
        delete *it;
        nodes.erase(it);
    }
}
