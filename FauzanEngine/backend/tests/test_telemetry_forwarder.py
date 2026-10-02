from __future__ import annotations

import httpx
import json
import pytest

from telemetry_forwarder import FarmTelemetryForwarder, TelemetryForwarderError


def envelope() -> dict:
    return {
        "schemaVersion": 1,
        "sourceRef": "farm-runtime-01",
        "gameKey": "farm-alpha",
        "engineVersion": "neo-test",
        "snapshotRef": "farm-runtime-01-1-2",
        "occurredAtMs": 123456,
        "telemetry": {"tileCount": 1000},
        "events": [
            {"eventRef": "farm-runtime-01-1-2-e-1", "eventType": "farm.harvested", "occurredAtMs": 123456}
        ],
    }


@pytest.mark.asyncio
async def test_forwarder_auth_and_server_side_bearer() -> None:
    seen = {}

    async def handler(request: httpx.Request) -> httpx.Response:
        seen["authorization"] = request.headers["authorization"]
        seen["payload"] = json.loads(request.content)
        return httpx.Response(202, json={"accepted": True})

    client = httpx.AsyncClient(transport=httpx.MockTransport(handler))
    forwarder = FarmTelemetryForwarder(
        control_plane_url="https://control-plane.test/api/runtime/farm",
        ingest_token="CONTROL_SECRET",
        runtime_token="RUNTIME_SECRET",
        client=client,
    )
    assert forwarder.authenticate_runtime("RUNTIME_SECRET")
    assert not forwarder.authenticate_runtime("wrong")
    status, body = await forwarder.forward(envelope())
    await client.aclose()
    assert status == 202
    assert body == {"accepted": True}
    assert seen["authorization"] == "Bearer CONTROL_SECRET"


@pytest.mark.asyncio
async def test_invalid_event_is_rejected_before_network() -> None:
    forwarder = FarmTelemetryForwarder(
        control_plane_url="https://control-plane.test/api/runtime/farm",
        ingest_token="CONTROL_SECRET",
        runtime_token="RUNTIME_SECRET",
    )
    bad = envelope()
    bad["events"][0]["eventType"] = "economy.write"
    with pytest.raises(TelemetryForwarderError):
        await forwarder.forward(bad)
