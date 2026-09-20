from __future__ import annotations

import json

from telemetry_store import TelemetryDurableStore, TelemetryStoreError


def make_envelope(ref: str = "event-1") -> dict:
    return {
        "schemaVersion": 1,
        "sourceRef": "farm-runtime-01",
        "gameKey": "farm-alpha",
        "engineVersion": "neo-test",
        "snapshotRef": "snap-1",
        "occurredAtMs": 123456,
        "telemetry": {"tileCount": 1000},
        "events": [{"eventRef": ref, "eventType": "farm.harvested", "occurredAtMs": 123456}],
    }


def test_durable_store_is_idempotent_and_acknowledgeable(tmp_path) -> None:
    store = TelemetryDurableStore(tmp_path / "telemetry.sqlite3")
    envelope = make_envelope()
    assert store.enqueue(envelope, 123456) == 1
    assert store.enqueue(envelope, 123456) == 0
    pending = store.pending()
    assert pending[0]["eventRef"] == "event-1"
    assert pending[0]["envelope"] == envelope
    assert store.pending_count() == 1
    assert store.acknowledge(["event-1"]) == 1
    assert store.pending_count() == 0


def test_durable_store_rejects_duplicate_refs(tmp_path) -> None:
    store = TelemetryDurableStore(tmp_path / "telemetry.sqlite3")
    envelope = make_envelope()
    envelope["events"].append(dict(envelope["events"][0]))
    try:
        store.enqueue(envelope, 123456)
        assert False
    except TelemetryStoreError:
        pass
