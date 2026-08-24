"""Local-only serializer for the C++ LocalAuthoringBridge v1 envelope.

No FastAPI route imports this module.  A trusted in-process/local caller must
explicitly opt in before a stored authoring document can become bridge bytes.
"""

from __future__ import annotations

import struct

from api.scene_documents import StoredSceneDocument


class LocalAuthoringApprovalRequired(PermissionError):
    """Raised before serialization when an explicit local approval is absent."""


_KINDS = {"empty": 0, "mesh": 1, "light": 2, "camera": 3, "player_start": 4, "marker": 5}


def _ascii(value: str, maximum: int, label: str) -> bytes:
    try:
        encoded = value.encode("ascii")
    except UnicodeEncodeError as error:
        raise ValueError(f"{label} must be ASCII") from error
    if not encoded or len(encoded) > maximum or any(byte < 0x21 or byte > 0x7E for byte in encoded):
        raise ValueError(f"{label} must contain 1-{maximum} printable ASCII bytes")
    return encoded


def _optional_ascii(value: str | None, maximum: int, label: str) -> bytes:
    return b"" if value is None else _ascii(value, maximum, label)


def serialize_local_authoring_handoff(document: StoredSceneDocument, *, approved: bool) -> bytes:
    """Return the exact bounded `NAB1` or `NAB2` payload consumed by C++ bridge."""
    if not approved:
        raise LocalAuthoringApprovalRequired("local authoring bridge approval is required")
    scene_id = _ascii(document.scene_id, 48, "scene_id")
    if document.version not in {1, 2} or document.revision < 1 or len(document.actors) > 512:
        raise ValueError("unsupported SceneDocument for local bridge")
    payload = bytearray(f"NAB{document.version}".encode("ascii"))
    payload.extend(struct.pack("<B", document.version))
    payload.extend(struct.pack("<B", len(scene_id)))
    payload.extend(scene_id)
    payload.extend(struct.pack("<QH", document.revision, len(document.actors)))
    for actor in document.actors:
        kind = _KINDS.get(actor.kind)
        if kind is None:
            raise ValueError("unsupported actor kind")
        asset_id = b"" if actor.asset_id is None else _ascii(actor.asset_id, 128, "asset_id")
        parent_id = 0 if actor.parent_id is None else actor.parent_id
        transform = actor.transform
        payload.extend(struct.pack(
            "<IIBfffffffffB",
            actor.id,
            parent_id,
            kind,
            transform.x,
            transform.y,
            transform.z,
            transform.rx,
            transform.ry,
            transform.rz,
            transform.sx,
            transform.sy,
            transform.sz,
            len(asset_id),
        ))
        payload.extend(asset_id)
        if document.version == 2:
            material_asset_id = _optional_ascii(actor.material_asset_id, 128, "material_asset_id")
            material_name = _optional_ascii(actor.material_name, 96, "material_name")
            texture_asset_id = _optional_ascii(actor.texture_asset_id, 128, "texture_asset_id")
            payload.extend(struct.pack("<B", len(material_asset_id)))
            payload.extend(material_asset_id)
            payload.extend(struct.pack("<B", len(material_name)))
            payload.extend(material_name)
            payload.extend(struct.pack("<B", len(texture_asset_id)))
            payload.extend(texture_asset_id)
    return bytes(payload)
