#include "Runtime/NeoRuntime.h"
#include <cassert>
int main() {
    NeoEngine::NeoRuntime runtime;
    NeoEngine::RuntimeConfig config{};
    config.farmNpcCount = 1;
    config.renderWidth = 64;
    config.renderHeight = 48;
    assert(runtime.Initialize(config));
    assert(runtime.ECS() != nullptr);
    assert(runtime.Scene() != nullptr);
    assert(runtime.SceneECS().sceneCount == runtime.Scene()->AliveCount());
    assert(runtime.SceneECS().ecsCount == runtime.Scene()->AliveCount());
    assert(runtime.Tick());
    assert(runtime.SceneECS().sceneCount == runtime.Scene()->AliveCount());
    assert(runtime.SceneECS().ecsCount == runtime.Scene()->AliveCount());
    assert(runtime.Shutdown());
    return 0;
}
