# Editor and Tooling P0–P2 Acceptance Matrix

## Scope

This document records the completion state of the editor/tooling priorities identified by the rescan. The work targets a functional Unreal-like authoring workflow over the existing NeoEngine editor surface. It does not claim complete Unreal Engine parity.

## P0 — Runtime Scene Bridge

`SceneBridge.ts` defines protocol version 1, revisioned documents, stable checksums, finite-transform and hierarchy validation, local fallback persistence, and optional HTTP delivery. `tools/scene-bridge.mjs` accepts only valid documents, rejects stale revisions with HTTP 409, rejects invalid scenes with HTTP 422, and returns an immutable receipt. Browser save can be verified against a running bridge by setting `VITE_SCENE_BRIDGE_URL` at Vite startup.

Acceptance command:

```bash
npm run bridge -- --port 8787
BRIDGE_URL=http://127.0.0.1:8787 npm run test:bridge
```

Expected: `SCENE_BRIDGE_SMOKE_OK commit=200 roundtrip=200 stale=409 invalid=422`.

## P1 — Authoring and Tooling

World Outliner supports single selection, Ctrl/Meta multi-selection, Select All, recursive hierarchy, inline rename, visibility toggle, reparenting through Details, and transactional multi-delete/multi-duplicate. Transform updates apply the active gizmo delta to the selection set and are captured in history.

Details uses `PropertySchema` metadata for component labels, type coercion, numeric ranges, read-only fields, and reset-to-default. Content Browser supports search, breadcrumb navigation, double-click spawn, and drag payloads. Viewport accepts asset drops and creates an asset-backed actor reference.

Automated browser acceptance command:

```bash
npm run dev -- --host 127.0.0.1 --port 4173
npm run test:browser
```

Expected: `EDITOR_BROWSER_SMOKE_OK initial=4 add=5 multi=2 transform=1 component=1 reparent=1 shortcut=1 save=1`.

## P2 — Runtime UX and Quality

Play-in-Editor uses the existing `GameRuntime` and displays an actual profiler overlay with elapsed time, actor state count, scripts, UI elements, input count, and logs. Dirty drafts are persisted every five seconds and at `beforeunload`; committed scenes clear the draft. Vite uses a vendor chunk boundary so the application chunk is approximately 161 KB rather than carrying the full Three/React dependency graph.

## Canonical C++ evidence

The canonical `editor_tooling_smoke` target proves hierarchy, selection, inspector, viewport rendering, transform mutation, undo/redo, save/load bytes, invalid-transform rollback, and deletion. It passes in Release and AddressSanitizer with leak detection enabled.

```bash
cmake -S Source/NeoEngine -B build/p0p2-release -DCMAKE_BUILD_TYPE=Release
cmake --build build/p0p2-release --target editor_tooling_smoke -j2
./build/p0p2-release/editor_tooling_smoke
```

## Residual boundaries

Remaining work is outside the completed P0–P2 editor/tooling slice: full C++ runtime ownership of every frontend scene mutation, multi-user collaboration, reflected C++ schema generation, material/animation/physics authoring panels, asset cooking/streaming, hot reload, package signing, Android evidence, and production readiness. These must be separate gates rather than implied by the editor UI.
