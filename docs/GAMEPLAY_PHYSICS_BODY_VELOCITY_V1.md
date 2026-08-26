# Gameplay Physics Body Velocity V1

## Scope

`GameplayPhysicsBodyBuilder::SetDynamicPlanarVelocity` provides a narrow caller-driven command for one existing canonical **dynamic** ECS circle body. The command validates finite requested planar velocity, a finite positive radius, and positive inverse mass before it commits both velocity components and exactly one ECS physics-revision signal.

| Request | Result |
|---|---|
| Live valid dynamic body | Commits `velocityX` and `velocityZ`, then marks physics dirty once. |
| Static body | Rejects with `StaticBody`; velocity and revision remain unchanged. |
| Unknown body, invalid stored body, or non-finite requested velocity | Rejects without mutating velocity or revision. |

The command writes ECS velocity only. It does not call XPBD, apply forces or impulses, resolve collisions, or write `SceneWorld` transforms.

## Evidence

The extended `gameplay_physics_body_smoke` proves one dynamic update changes both velocity components and increments the physics revision once. It also proves static, non-finite, and unknown requests preserve the revision, alongside existing static/dynamic snapshot and read-only query coverage.

## Boundary

This is not a Rigidbody API, a force/impulse layer, collision response, XPBD step owner, movement authority, or production physics claim.
