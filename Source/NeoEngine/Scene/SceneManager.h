#pragma once
#include <string>
#include <vector>
#include <memory>

namespace NeoEngine {
class SceneNode;
class SceneManager {
public:
    SceneManager() = default;
    void LoadScene(const std::string& name);
    void UnloadScene();
    void Update(float dt);
    SceneNode* GetRoot() const;
private:
    std::vector<std::unique_ptr<SceneNode>> nodes;
};
}
