#include "Runtime/CharacterPawn.h"
#include "Runtime/NeoRuntime.h"

#include <cmath>
#include <memory>

int main() {
    using namespace NeoEngine;
    NeoRuntime runtime;
    RuntimeConfig config{};
    config.enableFarmRuntimeHud = false;
    config.replicationRole = ReplicationRole::Server;
    config.replicationLocalClientId = 7U;
    if (!runtime.Initialize(config) || runtime.State() != RuntimeState::Initialized || runtime.Scene() == nullptr || runtime.Actors() == nullptr || runtime.Resources() == nullptr || runtime.Replication() == nullptr) return 1;

    SceneEntity actor{};
    if (!runtime.Actors()->CreateActor(actor, "RuntimeCharacter") || !runtime.Replication()->RegisterEntity(actor, 700U, 7U)) return 2;
    auto character = std::make_unique<CharacterPawn>();
    CharacterPawn* characterView = character.get();
    if (runtime.MotionAuthority() == nullptr || !characterView->BindMovementAuthorityGate(runtime.MotionAuthority()) || !runtime.Actors()->AttachComponent(actor, std::move(character)) || !characterView->SubmitInput({1.0F, 0.0F, false, false})) return 3;
    const Transform3* before = runtime.Scene()->GetTransform(actor);
    if (before == nullptr) return 4;
    const float beforeX = before->x;
    if (!runtime.Tick()) return 5;
    const Transform3* after = runtime.Scene()->GetTransform(actor);
    if (after == nullptr || after->x <= beforeX) return 6;

    ReplicationSnapshot snapshot{};
    if (!runtime.Replication()->BuildServerSnapshot(runtime.Clock()->Snapshot().fixedStepCount, snapshot) || snapshot.count != 1U || snapshot.states[0].networkId != 700U) return 7;
    if (!runtime.Assets()->ImportBytes("runtime.asset", AssetKind::Texture, {}, {1U, 2U, 3U}) || !runtime.Assets()->MarkReady("runtime.asset")) return 8;
    AssetResourceHandle resourceHandle{};
    if (!runtime.Resources()->Acquire("runtime.asset", resourceHandle) || runtime.Resources()->Data(resourceHandle) == nullptr || !runtime.Resources()->Release(resourceHandle)) return 9;

    if (!runtime.SetPaused(true)) return 10;
    const Transform3* beforePause = runtime.Scene()->GetTransform(actor);
    if (beforePause == nullptr) return 11;
    const float pauseX = beforePause->x;
    if (!characterView->SubmitInput({1.0F, 0.0F, false, false}) || !runtime.Tick()) return 12;
    const Transform3* afterPause = runtime.Scene()->GetTransform(actor);
    if (afterPause == nullptr || std::abs(afterPause->x - pauseX) > 0.0001F) return 13;
    if (!runtime.SetPaused(false) || !runtime.Shutdown() || runtime.State() != RuntimeState::Shutdown || runtime.Actors() != nullptr || runtime.Resources() != nullptr || runtime.Replication() != nullptr) return 14;
    return 0;
}
