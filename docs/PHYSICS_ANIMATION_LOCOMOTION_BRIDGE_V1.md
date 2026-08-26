# Physics Animation Locomotion Bridge V1

## Scope

`PhysicsAnimationLocomotionBridge` reads the validated planar velocity snapshot of one canonical ECS circle body and delegates that vector to the existing `AnimationLocomotionBridge`. The latter may select an explicit animation state through `AnimationStateMachine`; neither component owns movement or physics stepping.

| Stage | Effect |
|---|---|
| Body snapshot | Reads ECS position/velocity/collider state through `GameplayPhysicsBodyBuilder`; does not mark physics dirty. |
| Locomotion delegation | Uses only snapshot `velocityX` and `velocityZ` against the configured locomotion threshold. |
| Snapshot failure | Rejects before calling the locomotion bridge, preserving animation state and locomotion selection. |

The bridge contains no `SceneWorld`, `RouteIntent`, `MovementAuthorityGate`, `KinematicMotionController`, or XPBD `Step` call.

## Evidence

`physics_animation_locomotion_bridge_smoke` proves a dynamic body velocity selects move, a zero velocity selects idle, and an unknown-body snapshot failure preserves the previously active move state and locomotion flag without changing the ECS physics revision.

The target must pass in Release and AddressSanitizer with `ASAN_OPTIONS=detect_leaks=1`, followed by the broad non-Vulkan suite.

## Boundary

This is not skeletal animation, root motion, a Rigidbody layer, navigation, collision response, transform authority, or production animation readiness.
