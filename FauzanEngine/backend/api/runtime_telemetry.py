from fastapi import APIRouter, Header, HTTPException
from pydantic import BaseModel

import os

from telemetry_store import TelemetryDurableStore

from telemetry_forwarder import FarmTelemetryForwarder, TelemetryForwarderError

router = APIRouter(prefix="/api/runtime/farm", tags=["runtime-telemetry"])


class RuntimeTelemetryRequest(BaseModel):
    envelope: dict


@router.post("/forward")
async def forward_runtime_telemetry(
    request: RuntimeTelemetryRequest,
    x_runtime_forwarder_token: str | None = Header(default=None),
):
    store_path = os.getenv("ENGINE_TELEMETRY_DB", "").strip()
    if not store_path:
        raise HTTPException(status_code=503, detail="durable telemetry store is not configured")
    forwarder = FarmTelemetryForwarder(durable_store=TelemetryDurableStore(store_path))
    if not forwarder.authenticate_runtime(x_runtime_forwarder_token):
        raise HTTPException(status_code=401, detail="runtime authentication failed")
    try:
        status, body = await forwarder.forward(request.envelope)
    except TelemetryForwarderError as exc:
        raise HTTPException(status_code=400, detail=str(exc)) from exc
    if status >= 400:
        raise HTTPException(status_code=502, detail="control-plane ingest rejected envelope")
    return {"status": "forwarded", "upstreamStatus": status, "upstream": body}
