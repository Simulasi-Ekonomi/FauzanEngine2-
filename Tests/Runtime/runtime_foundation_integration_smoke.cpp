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

    if (!runtime.Assets()->ImportBytes("runtime.character", AssetKind::Prefab, {}, {4U, 5U, 6U}) || !runtime.Assets()->MarkReady("runtime.character")) return 2;
    AssetResourceHandle animationHandle{};
    if (!runtime.Resources()->Acquire("runtime.character", animationHandle) || runtime.Resources()->Data(animationHandle) == nullptr || runtime.Resources()->ActiveLeaseCount() != 1U) return 3;
    SceneEntity actor{};
    if (!runtime.Actors()->CreateActor(actor, "RuntimeCharacter") || !runtime.Replication()->RegisterEntity(actor, 700U, 7U)) return 4;
    auto character = std::make_unique<CharacterPawn>();
    CharacterPawn* characterView = character.get();
    if (runtime.MotionAuthority() == nullptr || !characterView->BindMovementAuthorityGate(runtime.MotionAuthority()) || !characterView->BindAnimationResource(runtime.Resources(), animationHandle) || !runtime.Actors()->AttachComponent(actor, std::move(character)) || !characterView->SubmitInput({1.0F, 0.0F, false, false})) return 5;
    const Transform3* before = runtime.Scene()->GetTransform(actor);
    if (before == nullptr) return 6;
    const float beforeX = before->x;
    if (!runtime.Tick()) return 7;
    const Transform3* after = runtime.Scene()->GetTransform(actor);
    if (after == nullptr || after->x <= beforeX) return 8;

    ActorComponentWorldSnapshot actorSnapshot{};
    if (!runtime.Actors()->CaptureSnapshot(actorSnapshot) || actorSnapshot.actors.size() != 1U || actorSnapshot.actors[0].componentCount != 1U || !actorSnapshot.begunPlay) return 9;
    ReplicationSnapshot snapshot{};
    if (!runtime.Replication()->BuildServerSnapshot(runtime.Clock()->Snapshot().fixedStepCount, snapshot) || snapshot.count != 1U || snapshot.states[0].networkId != 700U) return 10;
    if (!runtime.Assets()->ImportBytes("runtime.asset", AssetKind::Texture, {}, {1U, 2U, 3U}) || !runtime.Assets()->MarkReady("runtime.asset")) return 11;
    AssetResourceHandle resourceHandle{};
    if (!runtime.Resources()->Acquire("runtime.asset", resourceHandle) || runtime.Resources()->Data(resourceHandle) == nullptr || !runtime.Resources()->Release(resourceHandle)) return 12;

    if (!runtime.SetPaused(true)) return 13;
    const Transform3* beforePause = runtime.Scene()->GetTransform(actor);
    if (beforePause == nullptr) return 14;
    const float pauseX = beforePause->x;
    if (!characterView->SubmitInput({1.0F, 0.0F, false, false}) || !runtime.Tick()) return 15;
    const Transform3* afterPause = runtime.Scene()->GetTransform(actor);
    if (afterPause == nullptr || std::abs(afterPause->x - pauseX) > 0.0001F) return 16;
    if (!runtime.SetPaused(false) || !runtime.Actors()->DetachComponent(actor, CharacterPawn::kTypeId) || !runtime.Resources()->Release(animationHandle) || runtime.Resources()->ActiveLeaseCount() != 0U || !runtime.Shutdown() || runtime.State() != RuntimeState::Shutdown || runtime.Actors() != nullptr || runtime.Resources() != nullptr || runtime.Replication() != nullptr) return 17;
    return 0;
}
