#include "Runtime/NeoRuntime.h"
#include <cassert>
#include <cmath>
#include <limits>
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
    const auto entities = runtime.Scene()->AliveEntities();
    assert(!entities.empty());
    const NeoEngine::EntityID ecsId = runtime.SceneECSId(entities.front());
    assert(ecsId != std::numeric_limits<NeoEngine::EntityID>::max());
    float x=0.0F,y=0.0F,z=0.0F,rx=0.0F,ry=0.0F,rz=0.0F,sx=0.0F,sy=0.0F,sz=0.0F;
    assert(runtime.ECS()->TryGetPosition(ecsId,x,y,z));
    assert(runtime.ECS()->TryGetRotation(ecsId,rx,ry,rz));
    assert(runtime.ECS()->TryGetScale(ecsId,sx,sy,sz));
    assert(sx==1.0F && sy==1.0F && sz==1.0F);
    assert(std::isfinite(x) && std::isfinite(y) && std::isfinite(z));
    assert(std::isfinite(rx) && std::isfinite(ry) && std::isfinite(rz));
    assert(runtime.Tick());
    assert(runtime.SceneECS().sceneCount == runtime.Scene()->AliveCount());
    assert(runtime.SceneECS().ecsCount == runtime.Scene()->AliveCount());
    assert(runtime.Shutdown());
    return 0;
}
