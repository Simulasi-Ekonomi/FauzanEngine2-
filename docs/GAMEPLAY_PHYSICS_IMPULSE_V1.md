# Gameplay Physics Impulse V1

## Contract

`GameplayPhysicsBodyBuilder` provides `ApplyDynamicPlanarImpulse` and `ApplyDynamicPlanarImpulseSet` as bounded rigidbody-style gameplay helpers. Both paths locate an existing canonical ECS circle body and change only its `VelocityComponent` using `velocity += impulse × inverseMass`. They do not write `PositionComponent`, call XPBD step, synchronize SceneWorld transforms, alter collision layers, or bypass the established movement-authority boundary.

The batch path accepts at most 64 unique body IDs. It validates all identifiers, body state, static-body prohibition, finite impulses, and finite resulting velocities before one velocity write occurs. A static, duplicate, unknown, non-finite, invalid-state, or overflowing command rejects the whole batch without marking physics dirty or mutating an earlier body.

## Evidence and boundary

`gameplay_physics_body_smoke` and the direct `gameplay_physics_query_smoke` regression pass in Release and AddressSanitizer with `ASAN_OPTIONS=detect_leaks=1`. Evidence covers inverse-mass scaling, deterministic two-body batch mutation, static/duplicate/non-finite rejection, overflow rejection with velocity preservation, unchanged transform data, one subsequent XPBD step, and ray query behavior.

The API is not a complete rigidbody layer. It has no force accumulation over time, angular impulse, torque, constraints, restitution/friction authoring, trigger callbacks, network prediction, SceneWorld transform write authority, or production physics integration claim.
