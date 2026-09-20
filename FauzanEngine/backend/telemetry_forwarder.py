"""Trusted server-side Farm telemetry forwarder.

The engine never receives the control-plane ingest token. This module is the
authenticated host boundary: it authenticates the runtime handoff, validates
the bounded envelope shape, forwards it with the server-held control-plane
token, and returns the upstream response status/body for acknowledgement.
"""

from __future__ import annotations

import os
from typing import Any

import httpx


class TelemetryForwarderError(RuntimeError):
    pass


class FarmTelemetryForwarder:
    def __init__(
        self,
        *,
        control_plane_url: str | None = None,
        ingest_token: str | None = None,
        runtime_token: str | None = None,
        timeout_seconds: float = 10.0,
        client: httpx.AsyncClient | None = None,
    ) -> None:
        self.control_plane_url = (control_plane_url or os.getenv("CONTROL_PLANE_FARM_INGEST_URL", "")).strip()
        self.ingest_token = ingest_token if ingest_token is not None else os.getenv("ENGINE_TELEMETRY_INGEST_TOKEN", "")
        self.runtime_token = runtime_token if runtime_token is not None else os.getenv("ENGINE_RUNTIME_FORWARDER_TOKEN", "")
        self.timeout_seconds = timeout_seconds
        self._client = client

    def authenticate_runtime(self, supplied_token: str | None) -> bool:
        return bool(self.runtime_token) and supplied_token == self.runtime_token

    @staticmethod
    def validate_envelope(envelope: Any) -> dict[str, Any]:
        if not isinstance(envelope, dict):
            raise TelemetryForwarderError("envelope must be an object")
        required = ("schemaVersion", "sourceRef", "gameKey", "engineVersion", "snapshotRef", "occurredAtMs", "telemetry", "events")
        if any(key not in envelope for key in required):
            raise TelemetryForwarderError("telemetry envelope is missing required fields")
        if envelope["schemaVersion"] != 1:
            raise TelemetryForwarderError("unsupported telemetry schema")
        if not isinstance(envelope["events"], list) or len(envelope["events"]) > 64:
            raise TelemetryForwarderError("events must be a bounded list")
        if not isinstance(envelope["telemetry"], dict):
            raise TelemetryForwarderError("telemetry must be an object")
        for key in ("sourceRef", "gameKey", "engineVersion", "snapshotRef"):
            value = envelope[key]
            if not isinstance(value, str) or not value or len(value) > 128:
                raise TelemetryForwarderError(f"invalid {key}")
        if not isinstance(envelope["occurredAtMs"], int) or envelope["occurredAtMs"] <= 0:
            raise TelemetryForwarderError("invalid occurredAtMs")
        for event in envelope["events"]:
            if not isinstance(event, dict):
                raise TelemetryForwarderError("invalid event")
            if not isinstance(event.get("eventRef"), str) or not isinstance(event.get("eventType"), str):
                raise TelemetryForwarderError("eventRef/eventType are required")
            if not event["eventType"].startswith("farm."):
                raise TelemetryForwarderError("invalid eventType")
        return envelope

    async def forward(self, envelope: dict[str, Any]) -> tuple[int, dict[str, Any]]:
        payload = self.validate_envelope(envelope)
        if not self.control_plane_url or not self.ingest_token:
            raise TelemetryForwarderError("trusted control-plane configuration is incomplete")
        client = self._client
        owns_client = client is None
        if owns_client:
            client = httpx.AsyncClient(timeout=self.timeout_seconds)
        try:
            response = await client.post(
                self.control_plane_url,
                json=payload,
                headers={"Authorization": f"Bearer {self.ingest_token}"},
            )
            try:
                body = response.json()
            except ValueError:
                body = {"body": response.text[:4096]}
            return response.status_code, body
        except httpx.HTTPError as exc:
            raise TelemetryForwarderError("control-plane transport failed") from exc
        finally:
            if owns_client:
                await client.aclose()
