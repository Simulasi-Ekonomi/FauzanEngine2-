# World Authoring Navigation Rebuild V1

## Scope

`WorldAuthoring::Deserialize` now reconstructs a candidate `GridNavigation` from the decoded world payload before committing the restored authoring world. The rebuilt navigation reflects the decoded biome water cells, decoded tree blockers when enabled, and decoded building footprints rather than terrain generated from the configuration seed before payload assignment.

| Decode step | Validation and effect |
|---|---|
| Biomes | Retains only the bounded enum values from the full payload. |
| Trees | Requires unique forest cells inside the configured grid. |
| Buildings | Uses canonical placement validation against decoded terrain/tree state. |
| Navigation | Rebuilds a new grid from decoded water, trees, and buildings, then commits with the candidate world. |

Malformed payloads leave the existing target world intact because the entire decode executes into a local `WorldAuthoring` candidate.

## Evidence

The extended `world_authoring_smoke` keeps the deterministic round-trip and truncation checks, then changes one valid serialized unblocked forest cell into a water biome. It proves the decoded world reports that water biome and its rebuilt navigation blocks the same cell.

## Boundary

This is in-memory authoring-state reconstruction only. It does not create a runtime navigation agent, perform path following, synchronize multiplayer clients, read or write files, or claim production world-authoring readiness.
