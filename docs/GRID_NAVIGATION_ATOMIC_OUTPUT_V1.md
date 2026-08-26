# Grid Navigation Atomic Output V1

## Scope

`GridNavigation::FindPath` now builds a local route candidate and assigns the caller-provided route only after a complete successful path reconstruction. Validation and search failures no longer clear or partially replace an existing caller route.

| Failure | Caller route |
|---|---|
| Navigation not initialized | Preserved. |
| Start or goal out of bounds | Preserved. |
| Blocked endpoint | Preserved. |
| Unreachable goal or bounded reconstruction failure | Preserved. |

Successful routes retain their existing deterministic four-neighbor BFS order and include both start and goal.

## Evidence

The extended `grid_navigation_smoke` proves preservation of a sentinel route before initialization, then preservation of a valid routed detour after out-of-bounds, blocked-endpoint, and unreachable failures.

## Boundary

This changes only in-memory path-query output. It does not drive a runtime navigation agent, path-following, transform mutation, multiplayer navigation, or production navigation system.
