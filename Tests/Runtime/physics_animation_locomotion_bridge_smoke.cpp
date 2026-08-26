#include "Runtime/AnimationLocomotionBridge.h"
#include "Runtime/GameplayPhysicsBody.h"
#include "Runtime/PhysicsAnimationLocomotionBridge.h"

#include <cstdio>

int main() {
    using namespace NeoEngine;
    ArchetypeManager entities; GameplayPhysicsBodyBuilder bodies; EntityID dynamic = 0U;
    if (!bodies.CreateCircleBody(entities, {GameplayPhysicsBodyType::Dynamic, 0.0F, 0.0F, 2.0F, 0.0F, 0.5F, 1.0F}, dynamic)) return 1;
    AnimationStateMachine machine;
    if (!machine.AddState({"idle", "idle", AnimationPlayback::Clamp}) || !machine.AddState({"move", "move", AnimationPlayback::Clamp}) || !machine.AddTransition({"to-move", "idle", "move", 0.0F}) || !machine.AddTransition({"to-idle", "move", "idle", 0.0F}) || !machine.Start("idle")) return 1;
    AnimationLocomotionBridge locomotion;
    if (!locomotion.Initialize({"to-move", "to-idle", 0.1F})) return 1;
    PhysicsAnimationLocomotionBridge bridge;
    const uint64_t initialRevision = entities.GetPhysicsRevision();
    if (!bridge.Apply(entities, dynamic, bodies, locomotion, machine) || bridge.LastError() != PhysicsAnimationLocomotionBridgeError::None || machine.ActiveStateId() != "move" || !locomotion.IsLocomoting() || entities.GetPhysicsRevision() != initialRevision) return 1;
    if (!bodies.SetDynamicPlanarVelocity(entities, dynamic, 0.0F, 0.0F) || !bridge.Apply(entities, dynamic, bodies, locomotion, machine) || machine.ActiveStateId() != "idle" || locomotion.IsLocomoting()) return 1;
    if (!bodies.SetDynamicPlanarVelocity(entities, dynamic, 1.0F, 0.0F) || !bridge.Apply(entities, dynamic, bodies, locomotion, machine) || machine.ActiveStateId() != "move" || !locomotion.IsLocomoting()) return 1;
    const std::string preservedState = machine.ActiveStateId(); const bool preservedLocomotion = locomotion.IsLocomoting(); const uint64_t preservedRevision = entities.GetPhysicsRevision();
    if (bridge.Apply(entities, 99999U, bodies, locomotion, machine) || bridge.LastError() != PhysicsAnimationLocomotionBridgeError::SnapshotFailed || machine.ActiveStateId() != preservedState || locomotion.IsLocomoting() != preservedLocomotion || entities.GetPhysicsRevision() != preservedRevision) return 1;
    std::printf("PHYSICS_ANIMATION_LOCOMOTION_BRIDGE_SMOKE_OK move=1 idle=1 atomic=1 readOnly=1\n");
    return 0;
}
