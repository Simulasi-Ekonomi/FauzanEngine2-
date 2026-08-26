# Animation State Machine Snapshot V1

## Scope

`AnimationStateMachine::Snapshot` provides a caller-owned, read-only view of active state, optional blend target, normalized blend fraction, and scalar source/target clocks. It writes the caller output only after the machine is started and a complete candidate snapshot is assembled.

| Machine condition | Snapshot result |
|---|---|
| Not started | Rejects and preserves the caller snapshot. |
| Stable state | Reports active ID and scalar active clock; target ID is empty and blend is false. |
| Timed transition | Reports source ID, target ID, source/target clocks, and blend fraction in `[0,1]`. |
| Completed or immediate transition | Reports the new active ID with no blend target. |

## Evidence

`animation_state_machine_smoke` validates idle start, a timed idle-to-walk transition at zero and 50% blend, completed walk state, immediate walk-to-idle transition, invalid delta preservation, and not-started snapshot rejection.

## Authority boundary

The snapshot does not sample transforms, write `SceneWorld`, create route intent, step motion, change playback, or acquire `MovementAuthorityGate`. It is not skeletal blending, root motion, multi-segment route support, or runtime ownership.
