import struct
import unittest

from api.local_authoring_handoff import LocalAuthoringApprovalRequired, serialize_local_authoring_handoff
from api.scene_documents import SceneActorDocument, SceneDocumentPayload, SceneDocumentStore, SceneTransformDocument


class LocalAuthoringHandoffTests(unittest.TestCase):
    def document(self):
        store = SceneDocumentStore()
        return store.create(SceneDocumentPayload(scene_id="farm-slice", actors=[SceneActorDocument(id=10, kind="mesh", asset_id="mesh.cube", transform=SceneTransformDocument(x=4)), SceneActorDocument(id=20, parent_id=10, kind="marker", transform=SceneTransformDocument(x=2))]))

    def test_emits_bounded_nab1_payload(self) -> None:
        payload = serialize_local_authoring_handoff(self.document(), approved=True)
        self.assertEqual(payload[:4], b"NAB1")
        self.assertEqual(payload[4], 1)
        scene_size = payload[5]
        self.assertEqual(payload[6:6 + scene_size], b"farm-slice")
        revision, actor_count = struct.unpack_from("<QH", payload, 6 + scene_size)
        self.assertEqual((revision, actor_count), (1, 2))
        digest = 14695981039346656037
        for byte in payload:
            digest = (digest ^ byte) * 1099511628211 & 0xFFFFFFFFFFFFFFFF
        self.assertEqual(digest, 5832217868230341815)

    def test_requires_explicit_local_approval(self) -> None:
        with self.assertRaises(LocalAuthoringApprovalRequired):
            serialize_local_authoring_handoff(self.document(), approved=False)


if __name__ == "__main__":
    unittest.main()
