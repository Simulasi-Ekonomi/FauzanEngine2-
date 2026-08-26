# Gameplay Physics Query Batch V1

## Scope

`GameplayPhysicsQuery::OverlapCircleSet` evaluates one to 32 existing `GameplayOverlapCircle2` requests against XPBD's active flattened collider snapshot. Each inner overlap follows the established finite-shape, mask, capacity, entity-mapping, and ascending-`EntityID` rules. Caller output is replaced only after all queries succeed.

| Batch outcome | Output vector of entity sets |
|---|---|
| All 1–32 circles validate and map | Commits one ordered entity set per input circle. |
| Empty or more than 32 circles | Rejects without replacing caller output. |
| Any circle invalidates, exceeds hit capacity, or fails mapping | Rejects without replacing any prior output set. |

## Evidence

`gameplay_physics_query_smoke` proves a two-circle batch containing one ordered two-entity hit set and one empty set. It then proves a malformed second circle, empty batch, and 33-circle batch preserve the previously committed output.

## Boundary

This is a read-only query convenience API. It does not call XPBD `Step`, mutate ECS or `SceneWorld`, install trigger callbacks, resolve collisions, apply forces, or provide multiplayer authority.
