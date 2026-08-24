import unittest
from tempfile import TemporaryDirectory
from pathlib import Path

from api.scene_documents import (
    SceneActorDocument,
    SceneDocumentConflict,
    SceneDocumentPayload,
    SceneDocumentStore,
    SceneDocumentUpdate,
)


class SceneDocumentStoreTests(unittest.TestCase):
    def payload(self) -> SceneDocumentPayload:
        return SceneDocumentPayload(
            scene_id="farm-slice",
            actors=[
                SceneActorDocument(id=10, kind="mesh", asset_id="mesh.cube"),
                SceneActorDocument(id=20, parent_id=10, kind="marker"),
            ],
        )

    def test_create_read_update_and_revision_conflict(self) -> None:
        store = SceneDocumentStore()
        created = store.create(self.payload())
        self.assertEqual((created.revision, created.actor_count), (1, 2))
        self.assertEqual(store.read("farm-slice").checksum, created.checksum)
        updated = store.update(SceneDocumentUpdate(scene_id="farm-slice", actors=[SceneActorDocument(id=99, kind="light")], expected_revision=1))
        self.assertEqual((updated.revision, updated.actor_count), (2, 1))
        with self.assertRaises(SceneDocumentConflict):
            store.update(SceneDocumentUpdate(scene_id="farm-slice", actors=[], expected_revision=1))

    def test_invalid_parent_graph_is_rejected_by_schema(self) -> None:
        with self.assertRaises(ValueError):
            SceneDocumentPayload(scene_id="broken", actors=[SceneActorDocument(id=1, parent_id=2)])
        with self.assertRaises(ValueError):
            SceneDocumentPayload(scene_id="cycle", actors=[SceneActorDocument(id=1, parent_id=2), SceneActorDocument(id=2, parent_id=1)])

    def test_persists_records_with_revision_and_checksum(self) -> None:
        with TemporaryDirectory() as temporary:
            path = Path(temporary) / "authoring-scenes.json"
            created = SceneDocumentStore(path).create(self.payload())
            restored = SceneDocumentStore(path).read("farm-slice")
            self.assertIsNotNone(restored)
            self.assertEqual((restored.revision, restored.checksum), (created.revision, created.checksum))

    def test_v2_material_and_texture_bindings_are_versioned_and_persisted(self) -> None:
        payload = SceneDocumentPayload(
            version=2,
            scene_id="render-slice",
            actors=[SceneActorDocument(id=1, kind="mesh", asset_id="farm.mesh", material_asset_id="farm.material", material_name="grass", texture_asset_id="farm.texture")],
        )
        with TemporaryDirectory() as temporary:
            path = Path(temporary) / "authoring-scenes.json"
            created = SceneDocumentStore(path).create(payload)
            restored = SceneDocumentStore(path).read("render-slice")
            self.assertEqual((created.version, restored.version), (2, 2))
            self.assertEqual(restored.actors[0].material_name, "grass")
        with self.assertRaises(ValueError):
            SceneDocumentPayload(version=1, scene_id="legacy", actors=[SceneActorDocument(id=1, kind="mesh", material_asset_id="farm.material")])

    def test_v3_sprite_bindings_are_versioned_and_persisted(self) -> None:
        sprite = SceneActorDocument(
            id=1,
            kind="sprite",
            asset_id="farmer.texture",
            sprite_width=2.0,
            sprite_height=3.0,
            sprite_layer=4,
            sprite_order=5,
            sprite_rgba=0xFF28A0E0,
        )
        payload = SceneDocumentPayload(version=3, scene_id="sprite-slice", actors=[sprite])
        with TemporaryDirectory() as temporary:
            path = Path(temporary) / "authoring-scenes.json"
            restored = SceneDocumentStore(path).create(payload)
            self.assertEqual(restored.version, 3)
            self.assertEqual((restored.actors[0].sprite_width, restored.actors[0].sprite_order), (2.0, 5))
        with self.assertRaises(ValueError):
            SceneDocumentPayload(version=2, scene_id="legacy-sprite", actors=[sprite])
        with self.assertRaises(ValueError):
            SceneActorDocument(id=2, kind="sprite", sprite_width=1.0, sprite_height=1.0, sprite_layer=0, sprite_order=0, sprite_rgba=0xFFFFFFFF)


if __name__ == "__main__":
    unittest.main()
