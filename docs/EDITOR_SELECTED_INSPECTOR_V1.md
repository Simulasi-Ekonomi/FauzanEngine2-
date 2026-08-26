# Editor Selected Inspector V1

## Scope

`EditorSceneSession::InspectSelected` is a read-only convenience seam over the existing transient editor selection and canonical document inspector. It returns a snapshot of the currently selected **live document actor** only; it never returns a SceneWorld entity or mutable document reference.

| State | Result | Output boundary |
|---|---|---|
| A live actor is selected | Copies that actor's current document snapshot into caller output. | Commits the output snapshot and reports `None`. |
| No actor is selected | Returns false with `NoSelection`. | Does not modify caller output. |
| A selection somehow cannot resolve to a document actor | Delegates to canonical actor inspection. | Does not modify caller output. |

Selection remains transient: it is not serialized into `SceneDocument`, does not alter transforms, history, hierarchy, viewport rendering, or asset binding.

## Evidence

The extended `editor_scene_session_smoke` proves that the inspector rejects an unselected session with a sentinel actor unchanged, then returns the selected sprite actor snapshot with its texture asset. The existing selection/delete/open failure boundaries remain covered by the same Release and AddressSanitizer evidence.

## Boundary

This is not a desktop inspector UI, selection highlighting, filesystem project state, collaboration protocol, gameplay authority, or asset-editing interface.
