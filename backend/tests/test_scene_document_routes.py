import unittest
import os
from pathlib import Path
from tempfile import TemporaryDirectory

from fastapi import FastAPI
from fastapi.testclient import TestClient

_temporary_store = TemporaryDirectory()
os.environ["NEOENGINE_AUTHORING_STORE_PATH"] = str(Path(_temporary_store.name) / "authoring-scenes.json")

from api.routes import router


class SceneDocumentRouteTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        app = FastAPI()
        app.include_router(router)
        cls.client = TestClient(app)

    @classmethod
    def tearDownClass(cls) -> None:
        _temporary_store.cleanup()

    def test_create_read_update_and_conflict(self) -> None:
        payload = {
            "version": 1,
            "scene_id": "route-contract",
            "actors": [
                {"id": 10, "kind": "mesh", "asset_id": "mesh.cube"},
                {"id": 20, "parent_id": 10, "kind": "marker"},
            ],
        }
        created = self.client.post("/scene/documents", json=payload)
        self.assertEqual(created.status_code, 201)
        self.assertEqual((created.json()["revision"], created.json()["actor_count"]), (1, 2))
        read = self.client.get("/scene/documents/route-contract")
        self.assertEqual(read.status_code, 200)
        update = self.client.put("/scene/documents/route-contract", json={**payload, "expected_revision": 1, "actors": [{"id": 99, "kind": "light"}]})
        self.assertEqual((update.status_code, update.json()["revision"]), (200, 2))
        conflict = self.client.put("/scene/documents/route-contract", json={**payload, "expected_revision": 1})
        self.assertEqual(conflict.status_code, 409)

    def test_rejects_invalid_actor_graph(self) -> None:
        response = self.client.post("/scene/documents", json={"version": 1, "scene_id": "route-cycle", "actors": [{"id": 1, "parent_id": 2}, {"id": 2, "parent_id": 1}]})
        self.assertEqual(response.status_code, 422)


if __name__ == "__main__":
    unittest.main()
