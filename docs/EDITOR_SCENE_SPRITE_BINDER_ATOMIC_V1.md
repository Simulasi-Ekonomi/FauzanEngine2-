# Editor Scene Sprite Binder Atomic V1

## Scope

`EditorSceneSpriteBinder::BindDocumentAssets` now stages PPM/BMP texture resources into a candidate `TextureStagingStore` and assembles a candidate `SceneSpriteAdapter`. The caller's texture store and sprite target are replaced only after every eligible SceneDocument sprite actor validates, stages, maps to a scene entity, and registers successfully.

| Outcome | Caller texture store and sprite target |
|---|---|
| All bounded sprite actors succeed | Candidate texture resources and sprite adapter commit together. |
| A later sprite has an invalid/missing/unready/unmappable texture or scene binding | No earlier candidate texture staging reaches the caller and the existing sprite target remains unchanged. |

## Evidence

`editor_scene_sprite_binder_smoke` retains its normal staged PPM draw and transform proof, then creates a two-sprite document where the second ready texture has invalid bytes. The binder rejects with `TextureStageFailed`; an initially empty caller staging store remains empty and a copied pre-existing target retains one sprite.

## Boundary

This is an in-memory authoring-to-CPU-sprite binding transaction. It does not import files, persist editor data, create editor UI, own runtime scene execution, upload GPU textures, or establish production scene-loading readiness.
