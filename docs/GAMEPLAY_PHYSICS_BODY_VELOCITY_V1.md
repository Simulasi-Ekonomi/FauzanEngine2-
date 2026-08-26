# Gameplay Physics Body Velocity V1

## Scope

`GameplayPhysicsBodyBuilder::SetDynamicPlanarVelocity` provides a narrow caller-driven command for one existing canonical **dynamic** ECS circle body. The command validates finite requested planar velocity, a finite positive radius, and positive inverse mass before it commits both velocity components and exactly one ECS physics-revision signal.

`SetDynamicPlanarVelocitySet` applies from one to 64 distinct dynamic-body velocity commands. It resolves and validates every target before writing any velocity. A successful set writes all requested ECS velocities and marks the physics revision once; any invalid, duplicate, unknown, static, invalid-body, non-finite, empty, or oversized set leaves all velocities and revision unchanged.

| Request | Result |
|---|---|
| Live valid dynamic body | Commits `velocityX` and `velocityZ`, then marks physics dirty once. |
| 1–64 distinct live dynamic bodies | Commits all requested velocity pairs, then marks physics dirty once for the full set. |
| Static body | Rejects with `StaticBody`; velocity and revision remain unchanged. |
| Unknown body, invalid stored body, or non-finite requested velocity | Rejects without mutating velocity or revision. |

The command writes ECS velocity only. It does not call XPBD, apply forces or impulses, resolve collisions, or write `SceneWorld` transforms.

## Evidence

The extended `gameplay_physics_body_smoke` proves one dynamic update changes both velocity components and increments the physics revision once; it then proves a two-body batch increments that revision once for both ECS writes. A later static target, duplicate target, oversized set, and non-finite request preserve the valid bodies' prior velocity and revision, alongside existing static/dynamic snapshot and read-only query coverage.

## Boundary

This is not a Rigidbody API, a force/impulse layer, collision response, XPBD step owner, movement authority, or production physics claim.
