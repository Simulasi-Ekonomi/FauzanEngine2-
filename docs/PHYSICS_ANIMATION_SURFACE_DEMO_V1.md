# Physics Animation Surface Demo V1

## Scope

`RunPhysicsAnimationSurfaceDemo` is a finite four-or-more-frame software-rendered integration proof. For each frame, it sets the velocity of one dynamic ECS circle body, then uses `PhysicsAnimationLocomotionBridge` to select scalar move/idle animation tint. Separately, it delegates scene motion only through `KinematicMotionController` after acquiring `MovementAuthority::KinematicRoute`.

| Ownership | Proven behavior |
|---|---|
| ECS body velocity | Serves as the animation input source. |
| Physics-to-animation bridge | Reads the body snapshot and selects scalar locomotion only. |
| SceneWorld transform | Written only through `KinematicMotionController`. |
| Renderer | Reads the staged scene sprite and records a finite PPM artifact. |

The demo uses two moving-velocity frames followed by two zero-velocity frames. It retains the ending idle sample and counts exactly two move-tinted frames.

## Evidence

`physics_animation_surface_demo_smoke` rejects invalid dimensions, then proves four rendered and presented hidden-surface frames, nonzero pixels/hash, a P6 artifact, positive final kinematic X, two physics-sourced tint frames, and final idle animation state.

Release and AddressSanitizer execution with `ASAN_OPTIONS=detect_leaks=1` are required, followed by the broad non-Vulkan suite.

## Boundary

This is not skeletal animation, root motion, Rigidbody simulation, collision response, navigation, GPU rendering, a persistent game loop, APK, or production animation readiness.
