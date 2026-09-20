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


def test_durable_store_recovers_pending_events_after_reopen(tmp_path) -> None:
    db_path = tmp_path / "telemetry-recovery.sqlite3"
    envelope = make_envelope("event-recovery")

    first = TelemetryDurableStore(db_path)
    assert first.enqueue(envelope, 123456) == 1
    assert first.pending_count() == 1

    reopened = TelemetryDurableStore(db_path)
    pending = reopened.pending()
    assert pending == [{"eventRef": "event-recovery", "envelope": envelope}]
    assert reopened.pending_count() == 1

    assert reopened.acknowledge(["event-recovery"]) == 1
    final = TelemetryDurableStore(db_path)
    assert final.pending_count() == 0

def test_durable_store_enforces_total_pending_byte_budget(tmp_path) -> None:
    store = TelemetryDurableStore(tmp_path / "telemetry-bytes.sqlite3", max_bytes=180)
    envelope = make_envelope("event-bytes")
    try:
        store.enqueue(envelope, 123456)
        assert False
    except TelemetryStoreError:
        pass


def test_durable_store_retains_ack_tombstone_for_idempotent_replay(tmp_path) -> None:
    db_path = tmp_path / "telemetry-tombstone.sqlite3"
    store = TelemetryDurableStore(db_path)
    envelope = make_envelope("event-tombstone")
    assert store.enqueue(envelope, 123456) == 1
    assert store.acknowledge(["event-tombstone"]) == 1
    reopened = TelemetryDurableStore(db_path)
    assert reopened.enqueue(envelope, 123457) == 0
    assert reopened.pending_count() == 0


def test_durable_store_byte_budget_counts_each_new_event_and_allows_replay(tmp_path) -> None:
    store = TelemetryDurableStore(tmp_path / "telemetry-multi.sqlite3", max_bytes=520)
    envelope = make_envelope("event-a")
    envelope["events"].append({"eventRef": "event-b", "eventType": "farm.harvested", "occurredAtMs": 123456})
    try:
        store.enqueue(envelope, 123456)
        assert False
    except TelemetryStoreError:
        pass

    single = make_envelope("event-a")
    assert store.enqueue(single, 123456) == 1
    assert store.enqueue(single, 123457) == 0
