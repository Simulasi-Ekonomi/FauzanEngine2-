# Material Refresh Surface Atomic V1

## Scope

`MaterialRefreshSurfaceDemo` remains a finite hidden-SDL/software-rendered proof, but its MTL replacement now uses `AssetRefreshDiagnostics` plus `AssetRefreshExecutor::ExecuteAtomic`. The registry first receives replacement in-memory bytes; the diagnostics plan refreshes the staged material and rebinds the scene candidate. Both actions commit only after the full candidate execution succeeds.

| Frame | Required result |
|---|---|
| Initial frame | Green MTL material is bound to the copy-on-register scene mesh. |
| Atomic refresh | Replacement red MTL material and matching scene rebind commit together. |
| Final frames/artifact | The rendered hash and material receipt differ from the initial frame. |

## Evidence

`material_refresh_surface_demo_smoke` runs the finite two-frame hidden-surface demo, verifies nonempty visible output, different before/after framebuffer hashes, different material hashes and RGBA values, then validates the emitted PPM header. This now exercises the atomic refresh/rebind execution path end-to-end.

## Boundary

The demo uses caller-supplied in-memory bytes, a CPU renderer, and an optional finite SDL surface. It does not watch files, coordinate concurrent reloads, upload GPU material resources, persist a world, or establish production hot-reload behavior.
