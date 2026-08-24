#include "Runtime/NeoRuntime.h"

#include <cstdio>

using namespace NeoEngine;

namespace {

AuthorityCommand FarmCommand(const char* id, const char* kind, uint64_t sequence, uint64_t clientTick, uint16_t x, uint16_t z) {
    return {"runtime-farm-player", "runtime-farm-session", id, kind, sequence, clientTick, {static_cast<uint8_t>(x & 0xFFU), static_cast<uint8_t>(x >> 8U), static_cast<uint8_t>(z & 0xFFU), static_cast<uint8_t>(z >> 8U)}};
}

} // namespace

int main() {
    NeoRuntime runtime;
    bool ok = !runtime.Tick() && runtime.LastError() == RuntimeError::InvalidState;
    ok = ok && runtime.Initialize({20, 50, 2, 100, 128, 128, 8}) && runtime.State() == RuntimeState::Initialized;
    auto* authority = runtime.FarmAuthority();
    auto* world = runtime.FarmWorld();
    auto* assets = runtime.Assets();
    auto* scene = runtime.Scene();
    SceneEntity entity;
    ok = ok && authority && world && runtime.TrustSafety() && world->Snapshot().npcs == 8 && assets && assets->Declare("farm-tile", AssetKind::Texture, {}) && assets->MarkReady("farm-tile");
    ok = ok && scene && scene->Create(entity) && scene->SetTransform(entity, {1, 2, 3});
    ok = ok && authority->Submit(FarmCommand("runtime-cmd-001", "farm.till", 1, 1, 0, 0), 1).Accepted();
    ok = ok && authority->Submit(FarmCommand("runtime-cmd-002", "farm.plant.wheat", 2, 2, 0, 0), 2).Accepted();
    ok = ok && authority->Submit(FarmCommand("runtime-cmd-003", "farm.water", 3, 3, 0, 0), 3).Accepted();
    ok = ok && runtime.Tick() && runtime.RenderFarm() && runtime.Renderer()->FrameHash() != 0;
    ok = ok && runtime.Tick() && runtime.Tick() && runtime.Tick() && runtime.Tick() && runtime.Tick();
    ok = ok && authority->Submit(FarmCommand("runtime-cmd-004", "farm.harvest", 4, 4, 0, 0), 20).Accepted() && authority->LastHarvestedUnits() == 2;
    ok = ok && runtime.Shutdown() && runtime.State() == RuntimeState::Shutdown && runtime.FarmAuthority() == nullptr && runtime.FarmWorld() == nullptr && runtime.TrustSafety() == nullptr && runtime.Assets() == nullptr && runtime.Scene() == nullptr && runtime.Renderer() == nullptr && !runtime.Tick();
    if (!ok) {
        std::fprintf(stderr, "RUNTIME_SMOKE_FAIL\n");
        return 1;
    }
    std::printf("RUNTIME_SMOKE_OK lifecycle=init_tick_shutdown authority=owned scene=owned world=owned renderer=owned\n");
}
