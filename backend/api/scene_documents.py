"""Versioned, bounded authoring documents for the editor-to-runtime contract.

This module deliberately stores authoring drafts only.  It does not execute a
scene, publish a build, or provide player/economy authority.
"""

from __future__ import annotations

import hashlib
import json
import os
from dataclasses import dataclass
from pathlib import Path
from typing import Annotated

from pydantic import BaseModel, Field, field_validator, model_validator


SCENE_ID_MAX_BYTES = 48
SCENE_ACTOR_MAX_COUNT = 512


def _valid_asset_reference(value: str, label: str) -> str:
    if not value or len(value) > 128:
        raise ValueError(f"{label} must contain 1-128 characters")
    if not all(character.isascii() and 0x21 <= ord(character) <= 0x7E for character in value):
        raise ValueError(f"{label} only permits printable ASCII characters")
    return value


def _valid_scene_id(value: str) -> str:
    if not value or len(value) > SCENE_ID_MAX_BYTES:
        raise ValueError("scene_id must contain 1-48 characters")
    if not all(character.isascii() and (character.isalnum() or character in "_-") for character in value):
        raise ValueError("scene_id only permits ASCII letters, digits, '_' and '-'")
    return value


class SceneTransformDocument(BaseModel):
    x: float = 0.0
    y: float = 0.0
    z: float = 0.0
    rx: float = 0.0
    ry: float = 0.0
    rz: float = 0.0
    sx: float = 1.0
    sy: float = 1.0
    sz: float = 1.0

    @model_validator(mode="after")
    def validate_finite_positive_scale(self) -> "SceneTransformDocument":
        values = (self.x, self.y, self.z, self.rx, self.ry, self.rz, self.sx, self.sy, self.sz)
        if any(value != value or value in (float("inf"), float("-inf")) for value in values):
            raise ValueError("transform values must be finite")
        if self.sx <= 0.0 or self.sy <= 0.0 or self.sz <= 0.0:
            raise ValueError("transform scale must be positive")
        return self


class SceneActorDocument(BaseModel):
    id: Annotated[int, Field(gt=0)]
    parent_id: int | None = None
    kind: str = "empty"
    transform: SceneTransformDocument = Field(default_factory=SceneTransformDocument)
    asset_id: str | None = None
    material_asset_id: str | None = None
    material_name: str | None = None
    texture_asset_id: str | None = None

    @field_validator("kind")
    @classmethod
    def validate_kind(cls, value: str) -> str:
        if value not in {"empty", "mesh", "light", "camera", "player_start", "marker"}:
            raise ValueError("unsupported actor kind")
        return value

    @field_validator("asset_id")
    @classmethod
    def validate_asset_id(cls, value: str | None) -> str | None:
        return None if value is None else _valid_asset_reference(value, "asset_id")

    @field_validator("material_asset_id", "texture_asset_id")
    @classmethod
    def validate_optional_asset_reference(cls, value: str | None, info) -> str | None:
        return None if value is None else _valid_asset_reference(value, info.field_name)

    @field_validator("material_name")
    @classmethod
    def validate_material_name(cls, value: str | None) -> str | None:
        if value is None:
            return None
        if not value or len(value) > 96 or not all(character.isascii() and 0x21 <= ord(character) <= 0x7E for character in value):
            raise ValueError("material_name must contain 1-96 printable ASCII characters")
        return value

    @model_validator(mode="after")
    def validate_asset_binding(self) -> "SceneActorDocument":
        if self.material_name is not None and self.material_asset_id is None:
            raise ValueError("material_name requires material_asset_id")
        return self


class SceneDocumentPayload(BaseModel):
    version: Annotated[int, Field(ge=1, le=2)] = 2
    scene_id: str
    actors: list[SceneActorDocument] = Field(default_factory=list, max_length=SCENE_ACTOR_MAX_COUNT)

    @field_validator("scene_id")
    @classmethod
    def validate_scene_id(cls, value: str) -> str:
        return _valid_scene_id(value)

    @model_validator(mode="after")
    def validate_actor_graph(self) -> "SceneDocumentPayload":
        ids = {actor.id for actor in self.actors}
        if len(ids) != len(self.actors):
            raise ValueError("actor ids must be unique")
        parents = {actor.id: actor.parent_id for actor in self.actors}
        for actor in self.actors:
            if self.version == 1 and (actor.material_asset_id is not None or actor.material_name is not None or actor.texture_asset_id is not None):
                raise ValueError("SceneDocument v1 does not permit material or texture bindings")
            if actor.parent_id is not None and actor.parent_id not in ids:
                raise ValueError("parent_id must refer to an actor in the same document")
            seen: set[int] = set()
            current = actor.id
            while parents[current] is not None:
                if current in seen:
                    raise ValueError("actor graph must not contain a cycle")
                seen.add(current)
                current = parents[current]  # parent existence was checked above
        return self


class SceneDocumentUpdate(SceneDocumentPayload):
    expected_revision: Annotated[int, Field(ge=1)]


class SceneDocumentReceipt(BaseModel):
    scene_id: str
    revision: int
    checksum: str
    actor_count: int


class StoredSceneDocument(SceneDocumentReceipt):
    version: int
    actors: list[SceneActorDocument]


class SceneDocumentConflict(Exception):
    """Raised only when an optimistic revision precondition is not satisfied."""


@dataclass(frozen=True)
class _StoredRecord:
    version: int
    revision: int
    checksum: str
    actors: tuple[SceneActorDocument, ...]


class SceneDocumentStore:
    """In-memory authoring store with deterministic receipts.

    Storage lifetime is intentionally process-local until a durable backend
    storage contract is selected.  The API advertises no persistence guarantee.
    """

    def __init__(self, storage_path: Path | None = None) -> None:
        self._records: dict[str, _StoredRecord] = {}
        self._storage_path = storage_path
        if self._storage_path is not None:
            self._load()

    @classmethod
    def from_environment(cls) -> "SceneDocumentStore":
        configured = os.environ.get("NEOENGINE_AUTHORING_STORE_PATH", "backend/data/authoring-scenes.json")
        return cls(Path(configured))

    def _load(self) -> None:
        assert self._storage_path is not None
        if not self._storage_path.exists():
            return
        try:
            raw = json.loads(self._storage_path.read_text(encoding="utf-8"))
            if raw.get("version") != 1 or not isinstance(raw.get("scenes"), list):
                raise ValueError("unsupported authoring store format")
            for item in raw["scenes"]:
                payload = SceneDocumentPayload.model_validate({"version": item["version"], "scene_id": item["scene_id"], "actors": item["actors"]})
                revision = item["revision"]
                checksum = item["checksum"]
                if not isinstance(revision, int) or revision < 1 or not isinstance(checksum, str) or checksum != self._checksum(payload):
                    raise ValueError("invalid authoring store record")
                self._records[payload.scene_id] = _StoredRecord(payload.version, revision, checksum, tuple(payload.actors))
        except (OSError, KeyError, TypeError, ValueError) as error:
            raise RuntimeError(f"cannot load authoring scene store: {error}") from error

    def _persist(self) -> None:
        if self._storage_path is None:
            return
        self._storage_path.parent.mkdir(parents=True, exist_ok=True)
        payload = {
            "version": 1,
            "scenes": [
                {
                    "scene_id": scene_id,
                    "version": record.version,
                    "revision": record.revision,
                    "checksum": record.checksum,
                    "actors": [actor.model_dump(mode="json") for actor in record.actors],
                }
                for scene_id, record in sorted(self._records.items())
            ],
        }
        temporary = self._storage_path.with_name(f".{self._storage_path.name}.tmp")
        try:
            temporary.write_text(json.dumps(payload, sort_keys=True, separators=(",", ":")), encoding="utf-8")
            os.replace(temporary, self._storage_path)
        except OSError as error:
            temporary.unlink(missing_ok=True)
            raise RuntimeError(f"cannot persist authoring scene store: {error}") from error

    @staticmethod
    def _checksum(payload: SceneDocumentPayload) -> str:
        canonical = json.dumps(payload.model_dump(mode="json"), sort_keys=True, separators=(",", ":")).encode("utf-8")
        return hashlib.sha256(canonical).hexdigest()

    @staticmethod
    def _public(scene_id: str, record: _StoredRecord) -> StoredSceneDocument:
        return StoredSceneDocument(
            scene_id=scene_id,
            revision=record.revision,
            checksum=record.checksum,
            actor_count=len(record.actors),
            version=record.version,
            actors=list(record.actors),
        )

    def create(self, payload: SceneDocumentPayload) -> StoredSceneDocument:
        if payload.scene_id in self._records:
            raise SceneDocumentConflict("scene already exists")
        record = _StoredRecord(payload.version, 1, self._checksum(payload), tuple(payload.actors))
        self._records[payload.scene_id] = record
        self._persist()
        return self._public(payload.scene_id, record)

    def read(self, scene_id: str) -> StoredSceneDocument | None:
        record = self._records.get(_valid_scene_id(scene_id))
        return None if record is None else self._public(scene_id, record)

    def update(self, payload: SceneDocumentUpdate) -> StoredSceneDocument:
        current = self._records.get(payload.scene_id)
        if current is None:
            raise KeyError(payload.scene_id)
        if current.revision != payload.expected_revision:
            raise SceneDocumentConflict("revision conflict")
        next_payload = SceneDocumentPayload(version=payload.version, scene_id=payload.scene_id, actors=payload.actors)
        record = _StoredRecord(next_payload.version, current.revision + 1, self._checksum(next_payload), tuple(next_payload.actors))
        self._records[payload.scene_id] = record
        self._persist()
        return self._public(payload.scene_id, record)

    def delete(self, scene_id: str, expected_revision: int) -> None:
        current = self._records.get(_valid_scene_id(scene_id))
        if current is None:
            raise KeyError(scene_id)
        if current.revision != expected_revision:
            raise SceneDocumentConflict("revision conflict")
        del self._records[scene_id]
        self._persist()
