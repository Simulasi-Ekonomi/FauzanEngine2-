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


if __name__ == "__main__":
    unittest.main()
