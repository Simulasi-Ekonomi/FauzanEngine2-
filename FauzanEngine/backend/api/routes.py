"""
NeoEngine API Routes
REST endpoints for the editor frontend.
"""

from fastapi import APIRouter, Request
from pydantic import BaseModel


router = APIRouter()


class ChatRequest(BaseModel):
    message: str
    context: dict | None = None


class ChatResponse(BaseModel):
    response: str
    actions: list[dict] = []


class SceneActor(BaseModel):
    id: str | None = None
    name: str
    type: str
    transform: dict | None = None


@router.get("/status")
async def get_status(request: Request):
    """Get the overall system status."""
    brain = request.app.state.aries_brain
    manager = request.app.state.connection_manager
    return {
        "engine": "NeoEngine",
        "version": "1.0.0",
        "aries": brain.get_status(),
        "connections": manager.active_count(),
    }


@router.post("/aries/chat", response_model=ChatResponse)
async def aries_chat(req: ChatRequest, request: Request):
    """Send a message to Aries AI and get a response."""
    brain = request.app.state.aries_brain
    result = await brain.process_command(req.message, req.context)
    return ChatResponse(
        response=result.get("response", ""),
        actions=result.get("actions", []),
    )


@router.get("/aries/status")
async def aries_status(request: Request):
    """Get Aries AI brain status."""
    brain = request.app.state.aries_brain
    return brain.get_status()


@router.post("/aries/reset")
async def aries_reset(request: Request):
    """Reset Aries conversation history."""
    brain = request.app.state.aries_brain
    brain.conversation_history.clear()
    return {"status": "ok", "message": "Conversation history cleared"}


@router.get("/scene/actors")
async def get_scene_actors():
    """Get all actors in the current scene (placeholder)."""
    return {"actors": []}


@router.post("/scene/actors")
async def add_scene_actor(actor: SceneActor):
    """Add an actor to the scene (placeholder)."""
    return {"status": "ok", "actor": actor.model_dump()}


@router.delete("/scene/actors/{actor_id}")
async def remove_scene_actor(actor_id: str):
    """Remove an actor from the scene (placeholder)."""
    return {"status": "ok", "removed": actor_id}


@router.get("/engine/info")
async def engine_info():
    """Get engine information."""
    return {
        "name": "NeoEngine",
        "version": "1.0.0",
        "author": "Fauzan",
        "ai": "Aries Brain v3.0",
        "renderer": "Vulkan RHI",
        "platforms": ["Windows", "Linux", "Android"],
        "features": [
            "Entity Component System (ECS)",
            "Vulkan Rendering",
            "Physics Engine",
            "AI Agent System (150+ agents)",
            "Autonomous Game Builder",
            "WebSocket Bridge",
            "Aries AI Assistant",
        ],
    }
