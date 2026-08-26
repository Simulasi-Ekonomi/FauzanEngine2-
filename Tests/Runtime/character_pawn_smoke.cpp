#include "Runtime/ActorComponentWorld.h"
#include "Runtime/CharacterPawn.h"

#include <cmath>
#include <memory>
#include <string>
#include <vector>

int main() {
    using namespace NeoEngine;
    SceneWorld scene;
    ActorComponentWorld actors(scene);
    SceneEntity player{};
    if (!actors.CreateActor(player, "Player")) return 1;
    CharacterPawnConfig invalidConfig{};
    invalidConfig.fixedSeconds = 1.0F;
    invalidConfig.runSpeed = 101.0F;
    CharacterPawn invalidCharacter(invalidConfig);
    if (invalidCharacter.OnAttach(scene, player) || invalidCharacter.LastError() != CharacterPawnError::InvalidConfig || invalidCharacter.IsAttached()) return 1;

    auto character = std::make_unique<CharacterPawn>();
    CharacterPawn* characterView = character.get();
    CharacterAnimationGraph& graph = characterView->AnimationGraph();
    if (!graph.AddBaseState({"idle", "idle", AnimationPlayback::Loop}) || !graph.AddBaseState({"walk", "walk", AnimationPlayback::Loop}) || !graph.AddBaseState({"run", "run", AnimationPlayback::Loop}) || !graph.AddBaseTransition({"idle_walk", "idle", "walk", 0.10F}) || !graph.AddBaseTransition({"walk_idle", "walk", "idle", 0.10F}) || !graph.AddBaseTransition({"walk_run", "walk", "run", 0.10F}) || !graph.AddBaseTransition({"run_idle", "run", "idle", 0.10F}) || !graph.StartBase("idle")) return 2;
    if (!graph.AddOverlayState({"none", "none", AnimationPlayback::Loop}) || !graph.AddOverlayState({"aim", "aim", AnimationPlayback::Loop}) || !graph.AddOverlayTransition({"none_aim", "none", "aim", 0.20F}) || !graph.AddOverlayTransition({"aim_none", "aim", "none", 0.20F}) || !graph.StartOverlay("none") || !graph.SetOverlayWeightPermille(500U)) return 3;
    AnimationTimeline timeline;
    if (!timeline.AddTrack("idle", {{0.0F, 1.0F}, {1.0F, 1.0F}}) || !timeline.AddTrack("walk", {{0.0F, 2.0F}, {1.0F, 2.0F}}) || !timeline.AddTrack("run", {{0.0F, 3.0F}, {1.0F, 3.0F}}) || !timeline.AddTrack("none", {{0.0F, 0.0F}, {1.0F, 0.0F}}) || !timeline.AddTrack("aim", {{0.0F, 10.0F}, {1.0F, 10.0F}}) || !timeline.AddEventMarker("idle", {"idle_notify", 0.01F}) || !timeline.AddEventMarker("walk", {"walk_notify", 0.20F}) || !timeline.AddEventMarker("aim", {"aim_notify", 0.20F})) return 4;
    if (!actors.AttachComponent(player, std::move(character)) || !characterView->IsAttached()) return 5;
    if (!characterView->SetTransitionBinding({"idle", "walk", "idle_walk"}) || !characterView->SetTransitionBinding({"walk", "idle", "walk_idle"}) || !characterView->SetTransitionBinding({"walk", "run", "walk_run"}) || !characterView->SetTransitionBinding({"run", "idle", "run_idle"})) return 6;
    if (characterView->SetTransitionBinding({std::string("idle\0bad", 8U), "walk", "idle_walk"}) || characterView->LastError() != CharacterPawnError::AnimationRejected || !characterView->SetTransitionBinding({"idle", "walk", "idle_walk"})) return 6;

    ActorComponentWorldReceipt receipt{};
    if (!characterView->SubmitInput({}) || !actors.TickFixed(1U, receipt) || receipt.tickedComponents != 1U) return 6;
    std::vector<std::string> animationEvents{"sentinel"};
    if (!characterView->CollectAnimationEvents(timeline, 0.0F, 0.02F, animationEvents) || animationEvents.size() != 1U || animationEvents[0] != "idle_notify") return 7;
    animationEvents = {"preserved"};
    if (graph.CollectAnimationEvents(timeline, 1.0F, 0.0F, animationEvents) || animationEvents.size() != 1U || animationEvents[0] != "preserved") return 7;
    CharacterPawnSnapshot snapshot{};
    if (!characterView->Snapshot(snapshot) || snapshot.actor != player || snapshot.authority != CharacterMovementAuthority::KinematicRoute || !snapshot.grounded || snapshot.animation.base.activeStateId != "idle" || snapshot.animation.overlayWeightPermille != 500U) return 8;

    if (!characterView->SubmitInput({1.0F, 0.0F, false, false}) || !actors.TickFixed(1U, receipt) || !characterView->Snapshot(snapshot) || !snapshot.animation.base.blending || snapshot.velocity.x <= 0.0F) return 8;
    if (!graph.CollectAnimationEvents(timeline, 0.0F, 0.21F, animationEvents) || animationEvents.size() != 2U || animationEvents[0] != "idle_notify" || animationEvents[1] != "walk_notify") return 9;
    if (!characterView->TriggerOverlay("none_aim") || !actors.TickFixed(15U, receipt) || !characterView->Snapshot(snapshot) || snapshot.animation.hasOverlay == false || snapshot.animation.overlay.activeStateId != "aim") return 10;
    float compositeSample = 0.0F;
    if (!graph.Sample(timeline, compositeSample) || std::abs(compositeSample - 6.0F) > 0.0001F) return 11;
    if (!graph.CollectAnimationEvents(timeline, 0.0F, 0.30F, animationEvents) || animationEvents.size() != 2U || animationEvents[0] != "walk_notify" || animationEvents[1] != "aim_notify") return 11;
    const CharacterPawnSnapshot savedSnapshot = snapshot;
    CharacterAnimationGraphSnapshot invalidGraph = savedSnapshot.animation;
    invalidGraph.overlay.activeStateId = "missing";
    if (graph.Restore(invalidGraph) || graph.LastError() != AnimationStateMachineError::InvalidSnapshot || graph.ActiveBaseState() != savedSnapshot.animation.base.activeStateId || graph.ActiveOverlayState() != savedSnapshot.animation.overlay.activeStateId) return 12;
    CharacterPawnSnapshot invalidSnapshot = savedSnapshot;
    invalidSnapshot.actor.generation += 1U;
    if (characterView->Restore(invalidSnapshot) || characterView->LastError() != CharacterPawnError::AnimationRejected || !characterView->Snapshot(snapshot) || snapshot.animation.overlay.activeStateId != savedSnapshot.animation.overlay.activeStateId) return 12;
    invalidSnapshot = savedSnapshot;
    invalidSnapshot.grounded = true;
    invalidSnapshot.velocity.y = 1.0F;
    if (characterView->Restore(invalidSnapshot) || characterView->LastError() != CharacterPawnError::AnimationRejected || !characterView->Snapshot(snapshot) || snapshot.velocity.y != savedSnapshot.velocity.y) return 13;
    invalidSnapshot = savedSnapshot;
    invalidSnapshot.velocity.x = 101.0F;
    if (characterView->Restore(invalidSnapshot) || characterView->LastError() != CharacterPawnError::AnimationRejected || !characterView->Snapshot(snapshot) || snapshot.velocity.x != savedSnapshot.velocity.x) return 13;
    invalidSnapshot = savedSnapshot;
    invalidSnapshot.pendingInput.moveX = 2.0F;
    if (characterView->Restore(invalidSnapshot) || characterView->LastError() != CharacterPawnError::AnimationRejected || !characterView->Snapshot(snapshot) || snapshot.pendingInput.moveX != savedSnapshot.pendingInput.moveX) return 13;
    invalidSnapshot = savedSnapshot;
    invalidSnapshot.pendingRootMotion.x = 101.0F;
    if (characterView->Restore(invalidSnapshot) || characterView->LastError() != CharacterPawnError::AnimationRejected || !characterView->Snapshot(snapshot) || snapshot.pendingRootMotion.x != savedSnapshot.pendingRootMotion.x) return 13;
    if (!characterView->Restore(savedSnapshot) || !characterView->Snapshot(snapshot) || snapshot.animation.overlay.activeStateId != "aim" || snapshot.animation.overlayWeightPermille != 500U) return 14;
    if (!characterView->SubmitInput({0.25F, 0.0F, false, true}) || !characterView->SubmitRootMotion({})) return 14;
    ActorComponentWorldSnapshot worldSnapshot{};
    if (!actors.CaptureSnapshot(worldSnapshot) || worldSnapshot.actors.size() != 1U || worldSnapshot.actors[0].componentTypeNames[0] != "CharacterPawn" || worldSnapshot.componentBytes.size() != CharacterPawn::kComponentSnapshotBytes) return 14;
    ActorComponentWorldSnapshot invalidWorldSnapshot = worldSnapshot;
    invalidWorldSnapshot.componentBytes[0] = 0xFFU;
    if (actors.RestoreSnapshot(invalidWorldSnapshot) || actors.LastError() != ActorComponentError::RestoreRejected || !characterView->Snapshot(snapshot) || snapshot.velocity.x != savedSnapshot.velocity.x || snapshot.animation.overlay.activeStateId != "aim") return 14;
    if (!characterView->SubmitInput({1.0F, 0.0F, false, false}) || !actors.TickFixed(1U, receipt) || !actors.RestoreSnapshot(worldSnapshot) || !characterView->Snapshot(snapshot) || std::abs(snapshot.velocity.x - savedSnapshot.velocity.x) > 0.0001F || snapshot.pendingInput.moveX != 0.25F || !snapshot.pendingInput.jump || snapshot.animation.overlay.activeStateId != "aim" || snapshot.animation.overlayWeightPermille != 500U) return 14;
    if (!characterView->SubmitInput({0.0F, 0.0F, false, true}) || !actors.TickFixed(1U, receipt) || !characterView->Snapshot(snapshot) || snapshot.grounded || snapshot.velocity.y <= 0.0F) return 15;
    if (!actors.TickFixed(60U, receipt) || !characterView->Snapshot(snapshot) || !snapshot.grounded || std::abs(snapshot.velocity.y) > 0.0001F) return 16;

    const Transform3* beforeRoot = scene.GetTransform(player);
    if (beforeRoot == nullptr) return 14;
    const float beforeRootX = beforeRoot->x;
    if (!characterView->SetRootMotionMode(CharacterRootMotionMode::SkeletalRoot) || !characterView->SubmitRootMotion({0.25F, 0.0F, 0.0F}) || !actors.TickFixed(1U, receipt) || !characterView->Snapshot(snapshot) || snapshot.authority != CharacterMovementAuthority::SkeletalRoot) return 15;
    const Transform3* afterRoot = scene.GetTransform(player);
    if (afterRoot == nullptr || std::abs(afterRoot->x - beforeRootX - 0.25F) > 0.0001F) return 16;
    if (!characterView->SubmitInput({1.0F, 0.0F, false, false}) || !actors.TickFixed(1U, receipt) || !characterView->Snapshot(snapshot) || snapshot.authority != CharacterMovementAuthority::SkeletalRoot) return 17;
    if (!characterView->SetRootMotionMode(CharacterRootMotionMode::Kinematic) || !characterView->SubmitRootMotion({1.0F, 0.0F, 0.0F}) || actors.TickFixed(1U, receipt) || characterView->LastError() != CharacterPawnError::InvalidRootMotion) return 18;
    if (characterView->SubmitInput({2.0F, 0.0F, false, false}) || characterView->LastError() != CharacterPawnError::InvalidInput || !characterView->SubmitRootMotion({})) return 19;

    SceneEntity sharedPlayer{};
    MovementAuthorityGate sharedGate;
    if (!actors.CreateActor(sharedPlayer, "SharedGatePlayer")) return 18;
    auto sharedCharacter = std::make_unique<CharacterPawn>();
    CharacterPawn* sharedCharacterView = sharedCharacter.get();
    if (!sharedCharacterView->BindMovementAuthorityGate(&sharedGate) || !actors.AttachComponent(sharedPlayer, std::move(sharedCharacter)) || !sharedCharacterView->SubmitInput({1.0F, 0.0F, false, false})) return 19;
    const Transform3* sharedBefore = scene.GetTransform(sharedPlayer);
    if (sharedBefore == nullptr) return 20;
    const float sharedBeforeX = sharedBefore->x;
    sharedGate.BeginFrame();
    if (!sharedGate.Acquire(sharedPlayer, MovementAuthority::SkeletalRoot) || actors.TickFixed(1U, receipt) || sharedCharacterView->LastError() != CharacterPawnError::AuthorityRejected) return 21;
    const Transform3* sharedAfterReject = scene.GetTransform(sharedPlayer);
    if (sharedAfterReject == nullptr || std::abs(sharedAfterReject->x - sharedBeforeX) > 0.0001F) return 22;
    sharedGate.BeginFrame();
    if (!actors.TickFixed(1U, receipt) || !sharedCharacterView->Snapshot(snapshot) || snapshot.authority != CharacterMovementAuthority::KinematicRoute) return 23;
    if (!actors.DetachComponent(sharedPlayer, CharacterPawn::kTypeId) || !actors.DetachComponent(player, CharacterPawn::kTypeId) || actors.FindComponent(player, CharacterPawn::kTypeId) != nullptr || actors.ComponentCount(player) != 0U) return 24;
    if (!actors.TickFixed(1U, receipt) || receipt.tickedComponents != 0U) return 25;
    return 0;
}
