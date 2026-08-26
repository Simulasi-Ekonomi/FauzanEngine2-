#include "Runtime/ActorComponentWorld.h"
#include "Runtime/CharacterPawn.h"

#include <cmath>
#include <memory>

int main() {
    using namespace NeoEngine;
    SceneWorld scene;
    ActorComponentWorld actors(scene);
    SceneEntity player{};
    if (!actors.CreateActor(player, "Player")) return 1;

    auto character = std::make_unique<CharacterPawn>();
    CharacterPawn* characterView = character.get();
    CharacterAnimationGraph& graph = characterView->AnimationGraph();
    if (!graph.AddBaseState({"idle", "idle", AnimationPlayback::Loop}) || !graph.AddBaseState({"walk", "walk", AnimationPlayback::Loop}) || !graph.AddBaseState({"run", "run", AnimationPlayback::Loop}) || !graph.AddBaseTransition({"idle_walk", "idle", "walk", 0.10F}) || !graph.AddBaseTransition({"walk_idle", "walk", "idle", 0.10F}) || !graph.AddBaseTransition({"walk_run", "walk", "run", 0.10F}) || !graph.AddBaseTransition({"run_idle", "run", "idle", 0.10F}) || !graph.StartBase("idle")) return 2;
    if (!graph.AddOverlayState({"none", "none", AnimationPlayback::Loop}) || !graph.AddOverlayState({"aim", "aim", AnimationPlayback::Loop}) || !graph.AddOverlayTransition({"none_aim", "none", "aim", 0.20F}) || !graph.AddOverlayTransition({"aim_none", "aim", "none", 0.20F}) || !graph.StartOverlay("none")) return 3;
    if (!actors.AttachComponent(player, std::move(character)) || !characterView->IsAttached()) return 4;
    if (!characterView->SetTransitionBinding({"idle", "walk", "idle_walk"}) || !characterView->SetTransitionBinding({"walk", "idle", "walk_idle"}) || !characterView->SetTransitionBinding({"walk", "run", "walk_run"}) || !characterView->SetTransitionBinding({"run", "idle", "run_idle"})) return 5;

    ActorComponentWorldReceipt receipt{};
    if (!characterView->SubmitInput({}) || !actors.TickFixed(1U, receipt) || receipt.tickedComponents != 1U) return 6;
    CharacterPawnSnapshot snapshot{};
    if (!characterView->Snapshot(snapshot) || snapshot.actor != player || snapshot.authority != CharacterMovementAuthority::KinematicRoute || !snapshot.grounded || snapshot.animation.base.activeStateId != "idle") return 7;

    if (!characterView->SubmitInput({1.0F, 0.0F, false, false}) || !actors.TickFixed(1U, receipt) || !characterView->Snapshot(snapshot) || !snapshot.animation.base.blending || snapshot.velocity.x <= 0.0F) return 8;
    if (!characterView->TriggerOverlay("none_aim") || !actors.TickFixed(15U, receipt) || !characterView->Snapshot(snapshot) || snapshot.animation.hasOverlay == false || snapshot.animation.overlay.activeStateId != "aim") return 9;
    if (!characterView->SubmitInput({0.0F, 0.0F, false, true}) || !actors.TickFixed(1U, receipt) || !characterView->Snapshot(snapshot) || snapshot.grounded || snapshot.velocity.y <= 0.0F) return 10;
    if (!actors.TickFixed(60U, receipt) || !characterView->Snapshot(snapshot) || !snapshot.grounded || std::abs(snapshot.velocity.y) > 0.0001F) return 11;

    const Transform3* beforeRoot = scene.GetTransform(player);
    if (beforeRoot == nullptr) return 12;
    const float beforeRootX = beforeRoot->x;
    if (!characterView->SetRootMotionMode(CharacterRootMotionMode::SkeletalRoot) || !characterView->SubmitRootMotion({0.25F, 0.0F, 0.0F}) || !actors.TickFixed(1U, receipt) || !characterView->Snapshot(snapshot) || snapshot.authority != CharacterMovementAuthority::SkeletalRoot) return 13;
    const Transform3* afterRoot = scene.GetTransform(player);
    if (afterRoot == nullptr || std::abs(afterRoot->x - beforeRootX - 0.25F) > 0.0001F) return 14;
    if (!characterView->SubmitInput({1.0F, 0.0F, false, false}) || !actors.TickFixed(1U, receipt) || !characterView->Snapshot(snapshot) || snapshot.authority != CharacterMovementAuthority::SkeletalRoot) return 15;
    if (!characterView->SetRootMotionMode(CharacterRootMotionMode::Kinematic) || !characterView->SubmitRootMotion({1.0F, 0.0F, 0.0F}) || actors.TickFixed(1U, receipt) || characterView->LastError() != CharacterPawnError::InvalidRootMotion) return 16;
    if (characterView->SubmitInput({2.0F, 0.0F, false, false}) || characterView->LastError() != CharacterPawnError::InvalidInput) return 17;
    if (!actors.DetachComponent(player, CharacterPawn::kTypeId) || actors.FindComponent(player, CharacterPawn::kTypeId) != nullptr || actors.ComponentCount(player) != 0U) return 18;
    if (!actors.TickFixed(1U, receipt) || receipt.tickedComponents != 0U) return 19;
    return 0;
}
