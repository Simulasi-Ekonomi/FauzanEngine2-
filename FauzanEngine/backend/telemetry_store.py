"""Durable local persistence for bounded runtime telemetry envelopes."""

from __future__ import annotations

import json
import sqlite3
import threading
from pathlib import Path
from typing import Any


class TelemetryStoreError(RuntimeError):
    pass


class TelemetryDurableStore:
    """Crash-safe SQLite queue with idempotent event references."""

    def __init__(self, path: str | Path, *, max_events: int = 100_000, max_bytes: int = 64 * 1024 * 1024) -> None:
        if max_events <= 0:
            raise ValueError("max_events must be positive")
        if max_bytes <= 0:
            raise ValueError("max_bytes must be positive")
        self.path = str(path)
        self.max_events = max_events
        self.max_bytes = max_bytes
        self._lock = threading.Lock()
        parent = Path(self.path).parent
        if str(parent) not in ("", "."):
            parent.mkdir(parents=True, exist_ok=True)
        with self._connect() as db:
            db.execute("PRAGMA journal_mode=WAL")
            db.execute("PRAGMA synchronous=FULL")
            db.execute(
                """CREATE TABLE IF NOT EXISTS telemetry_events (
                    event_ref TEXT PRIMARY KEY,
                    envelope_json TEXT NOT NULL,
                    state TEXT NOT NULL CHECK(state IN ('pending','acked')),
                    created_at INTEGER NOT NULL
                )"""
            )
            db.execute("CREATE INDEX IF NOT EXISTS idx_telemetry_state ON telemetry_events(state)")

    def _connect(self) -> sqlite3.Connection:
        db = sqlite3.connect(self.path, timeout=5.0)
        db.execute("PRAGMA foreign_keys=ON")
        return db

    @staticmethod
    def _event_refs(envelope: dict[str, Any]) -> list[str]:
        refs = []
        for event in envelope.get("events", []):
            ref = event.get("eventRef") if isinstance(event, dict) else None
            if not isinstance(ref, str) or not ref:
                raise TelemetryStoreError("eventRef is required")
            refs.append(ref)
        if len(refs) != len(set(refs)):
            raise TelemetryStoreError("duplicate eventRef in envelope")
        return refs

    def enqueue(self, envelope: dict[str, Any], occurred_at_ms: int) -> int:
        payload = json.dumps(envelope, separators=(",", ":"), sort_keys=True)
        if len(payload.encode("utf-8")) > self.max_bytes:
            raise TelemetryStoreError("telemetry envelope exceeds durable-store byte limit")
        refs = self._event_refs(envelope)
        if not refs:
            return 0
        with self._lock, self._connect() as db:
            pending = db.execute("SELECT COUNT(*) FROM telemetry_events WHERE state='pending'").fetchone()[0]
            if pending + len(refs) > self.max_events:
                raise TelemetryStoreError("telemetry durable queue is full")
            pending_bytes = db.execute(
                "SELECT COALESCE(SUM(length(CAST(envelope_json AS BLOB))), 0) "
                "FROM telemetry_events WHERE state='pending'"
            ).fetchone()[0]
            if pending_bytes + len(payload.encode("utf-8")) > self.max_bytes:
                raise TelemetryStoreError("telemetry durable queue byte limit exceeded")
            inserted = 0
            for ref in refs:
                cur = db.execute(
                    "INSERT OR IGNORE INTO telemetry_events(event_ref,envelope_json,state,created_at) VALUES(?,?,?,?)",
                    (ref, payload, "pending", occurred_at_ms),
                )
                inserted += cur.rowcount
            return inserted

    def pending(self, limit: int = 64) -> list[dict[str, Any]]:
        if limit <= 0:
            return []
        with self._lock, self._connect() as db:
            rows = db.execute(
                "SELECT event_ref,envelope_json FROM telemetry_events WHERE state='pending' ORDER BY created_at,event_ref LIMIT ?",
                (min(limit, self.max_events),),
            ).fetchall()
        return [{"eventRef": ref, "envelope": json.loads(payload)} for ref, payload in rows]

    def acknowledge(self, event_refs: list[str]) -> int:
        refs = [r for r in event_refs if isinstance(r, str) and r]
        if not refs:
            return 0
        with self._lock, self._connect() as db:
            cur = db.executemany(
                "UPDATE telemetry_events SET state='acked' WHERE event_ref=? AND state='pending'",
                ((ref,) for ref in refs),
            )
            return cur.rowcount

    def pending_count(self) -> int:
        with self._lock, self._connect() as db:
            return db.execute("SELECT COUNT(*) FROM telemetry_events WHERE state='pending'").fetchone()[0]
