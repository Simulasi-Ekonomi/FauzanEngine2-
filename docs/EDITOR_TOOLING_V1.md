# Editor Tooling V1

## Goal

Editor Tooling V1 provides a bounded Unreal-like authoring workflow over the existing NeoEngine editor surface. The workflow is intentionally honest: it delivers a functional scene authoring UI and a canonical in-engine editor-session smoke path, but it does not claim full Unreal Engine parity or production readiness.

## Delivered workflow

The editor surface now provides:

- World Outliner selection with recursive hierarchy display, search, inline rename on double-click, context selection, and per-actor visibility toggle.
- Details/Inspector editing for actor name, location, rotation, scale, component properties, and adding a generic scene component.
- Viewport authoring with grid, orbit camera, axis gizmo, selectable actors, hidden-actor filtering, transform gizmo, translate/rotate/scale modes, world/local space, perspective/top/front/right camera presets, and play-mode separation.
- Scene lifecycle controls for new scene, local save, JSON export/import, dirty marker, saved timestamp, undo, redo, duplicate, and delete.
- Editor shortcuts: W/E/R for transform mode, 1/2/3/4 for camera presets, Ctrl+S for save, Ctrl+Z/Ctrl+Y or Ctrl+Shift+Z for history, Ctrl+D for duplicate, and Delete for the selected actor.
- Status evidence for FPS, actor count, selected actor, runtime/AI connection, scene name, and saved/unsaved state.

## Invariants

Scene mutations are recorded through the same Zustand scene store, use bounded undo snapshots, update dirty state, and preserve parent-child references when reparenting. Invalid hierarchy cycles are rejected. The canonical C++ editor session remains the authoritative validation boundary for scene open, selection, inspector, transform mutation, viewport rendering, save/load, undo/redo, deletion, and failure-preserving rollback.

## Browser acceptance evidence

`EDITOR_TOOLING_V1_BROWSER_EVIDENCE.md` records a live preview verification of the rendered Unreal-like surface, toolbar actor creation, dirty-state propagation, inspector transform editing, component addition, and parent reparenting in the World Outliner.

## Acceptance test

The canonical smoke target is `editor_tooling_smoke`. It proves hierarchy ordering, selection, inspector data, viewport rendering, transform mutation, undo, redo, save/load byte handoff, invalid-transform rollback, and deletion. The frontend acceptance gate is the strict TypeScript/Vite production build.

```bash
cd FauzanEngine/editor
npm run build

cd ../..
cmake -S Source/NeoEngine -B build/editor-v1 -DCMAKE_BUILD_TYPE=Release
cmake --build build/editor-v1 --target editor_tooling_smoke -j2
./build/editor-v1/editor_tooling_smoke
```

Expected smoke output begins with `EDITOR_TOOLING_SMOKE_OK` and includes `hierarchy=1 selection=1 inspector=1 viewport=1 transform=1 undo=1 redo=1 save=1 load=1 rollback=1 delete=1`.

## Explicit residual gap

This is an Editor Tooling V1 slice, not the full Unreal Editor. Asset cooking/streaming, a complete GPU scene presentation path, multi-selection, property metadata/schema reflection, prefab authoring UI, material/animation/physics editors, collaboration, hot reload, packaging, and production readiness remain outside this checkpoint.
