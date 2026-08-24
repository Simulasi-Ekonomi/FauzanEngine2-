#pragma once
#include <vector>
#include <algorithm>

namespace NeoEngine {
struct SceneNode;
class SceneGraph {
public:
    SceneGraph() = default;
    ~SceneGraph() = default;
    void AddNode(SceneNode* node);
    void RemoveNode(SceneNode* node);
private:
    std::vector<SceneNode*> nodes;
};
}
