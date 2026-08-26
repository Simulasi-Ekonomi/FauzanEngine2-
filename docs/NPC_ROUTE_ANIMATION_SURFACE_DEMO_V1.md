# NPC Route Animation Surface Demo V1

## Scope

`RunNpcRouteAnimationSurfaceDemo` is a finite five-or-more-frame software-rendered proof that composes a three-cell grid route, an ECS body velocity snapshot, scalar locomotion animation, and a staged sprite. It is intentionally caller-driven and bounded.

| Concern | Owner in the proof |
|---|---|
| Route target query | `GridRouteFollower::PeekIntent`, read-only. |
| SceneWorld movement | `GridRouteFollower::StepGuarded`, which delegates to `KinematicMotionController` under `MovementAuthority::KinematicRoute`. |
| ECS velocity | A dynamic gameplay circle body, set from the current route intent direction. |
| Animation selection | `PhysicsAnimationLocomotionBridge`, which reads ECS velocity only. |
| Visual result | Frame-local sprite tint through `AnimationSpriteTintBinding`. |

The first four frames traverse two route cells at 2 units/s with 0.25-second steps. The fifth frame supplies zero velocity after goal arrival and proves final idle selection.

## Evidence

`npc_route_animation_surface_demo_smoke` rejects an invalid four-frame configuration, then proves five rendered and presented frames, a P6 artifact, four move-tinted frames, final `(1,2)` route goal, and final idle animation state.

## Boundary

This is not a behavior tree, an autonomous navigation agent, skeletal animation, multiplayer NPC synchronization, persistent host loop, GPU renderer, or production NPC system.
