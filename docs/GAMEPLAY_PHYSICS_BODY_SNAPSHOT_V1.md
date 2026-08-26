# Gameplay Physics Body Snapshot V1

## Scope

`GameplayPhysicsBodyBuilder::SnapshotCircleBody` exposes a validated, copied read-only snapshot of a canonical ECS circle body. It scans the ECS position, velocity, and collider components but does not step XPBD, mutate ECS, change the physics revision, or write a `SceneWorld` transform.

| Body state | Snapshot result |
|---|---|
| Static (`inverseMass == 0`) | Requires zero planar velocity and returns a static snapshot. |
| Dynamic (`inverseMass > 0`) | Returns position, planar velocity, radius, and inverse mass after finite-value validation. |
| Missing or structurally invalid body | Returns `UnknownBody` or `InvalidBodyState` without replacing caller output. |

The API deliberately exposes inverse mass instead of reconstructing or inferring a gameplay mass contract.

## Evidence

The extended `gameplay_physics_body_smoke` proves dynamic and static snapshots alongside existing ECS construction and read-only query evidence. It verifies an unknown entity leaves a sentinel output intact and does not change the ECS physics revision.

## Boundary

This is not a Rigidbody API, collision response system, XPBD stepping owner, force/impulse interface, transform authority, or production physics claim.
