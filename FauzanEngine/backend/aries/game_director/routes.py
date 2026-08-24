"""
FastAPI routes untuk Game Director
Expose Aries/Hermes/Ruflo ke frontend via WebSocket + REST
"""

import asyncio
import json
import os
from fastapi import APIRouter, WebSocket, WebSocketDisconnect, HTTPException
from fastapi.responses import StreamingResponse
from pydantic import BaseModel
from ..agents.aries_director import AriesDirector

router = APIRouter(prefix="/game-director", tags=["Game Director"])

# Singleton director per session
directors: dict[str, AriesDirector] = {}

def get_director(session_id: str = "default") -> AriesDirector:
    if session_id not in directors:
        directors[session_id] = AriesDirector()
    return directors[session_id]


class ChatRequest(BaseModel):
    message: str
    session_id: str = "default"


class GameRequest(BaseModel):
    prompt: str
    session_id: str = "default"


@router.post("/chat")
async def chat(req: ChatRequest):
    """Chat langsung dengan Aries"""
    if not os.environ.get("GITHUB_TOKEN"):
        raise HTTPException(503, "GITHUB_TOKEN tidak di-set")
    
    director = get_director(req.session_id)
    
    async def generate():
        async for token in director.chat(req.message):
            yield f"data: {json.dumps({'token': token})}\n\n"
        yield "data: [DONE]\n\n"
    
    return StreamingResponse(generate(), media_type="text/event-stream")


@router.websocket("/create-game")
async def create_game_ws(websocket: WebSocket):
    """
    WebSocket endpoint untuk create game
    Kirim prompt, terima progress updates real-time
    """
    await websocket.accept()
    
    try:
        # Terima prompt
        data = await websocket.receive_json()
        prompt = data.get("prompt", "")
        session_id = data.get("session_id", "default")
        
        if not prompt:
            await websocket.send_json({"error": "Prompt kosong"})
            return
        
        if not os.environ.get("GITHUB_TOKEN"):
            await websocket.send_json({"error": "GITHUB_TOKEN tidak di-set di server"})
            return
        
        director = get_director(session_id)
        director.reset()
        
        # Stream semua progress
        async for update in director.create_game(prompt):
            await websocket.send_json(update)
            
            # Beri jeda kecil agar tidak flood
            await asyncio.sleep(0.01)
        
        await websocket.send_json({"phase": "complete", "message": "Game generation selesai!"})
        
    except WebSocketDisconnect:
        pass
    except Exception as e:
        await websocket.send_json({"error": str(e)})


@router.get("/status/{session_id}")
async def get_status(session_id: str):
    """Cek status game generation"""
    if session_id not in directors:
        return {"status": "idle"}
    
    d = directors[session_id]
    return {
        "status": "active",
        "completed_tasks": len(d.completed),
        "total_tasks": len(d.task_queue) + len(d.completed),
        "gdd_ready": bool(d.gdd)
    }


@router.delete("/reset/{session_id}")
async def reset_session(session_id: str):
    """Reset session game director"""
    if session_id in directors:
        directors[session_id].reset()
    return {"status": "reset"}
