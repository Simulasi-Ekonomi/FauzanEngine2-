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

# =========================================================
# 3D Asset Generation Endpoint (TripoSR / AI Model)
# =========================================================
from fastapi import BackgroundTasks
import subprocess
import tempfile
import os

@router.post("/generate-3d")
async def generate_3d_asset(prompt: str, background_tasks: BackgroundTasks):
    """
    Generate 3D mesh from text prompt using TripoSR (local) or fallback.
    Returns a GLB file.
    """
    # Simpan prompt ke file sementara
    with tempfile.NamedTemporaryFile(mode='w', suffix='.txt', delete=False) as f:
        f.write(prompt)
        prompt_file = f.name

    output_file = tempfile.NamedTemporaryFile(suffix='.glb', delete=False)
    output_path = output_file.name
    output_file.close()

    async def run_generation():
        # Panggil script TripoSR (asumsi sudah terinstall di environment)
        cmd = [
            "python", "-m", "triposr",
            "--prompt", prompt,
            "--output", output_path,
            "--format", "glb"
        ]
        subprocess.run(cmd, capture_output=True)
        # Cleanup prompt file
        os.unlink(prompt_file)

    background_tasks.add_task(run_generation)

    # Return job ID atau langsung file jika cepat
    return {"status": "processing", "output": output_path}

# =========================================================
# Massive World Generation Endpoint
# =========================================================
from fastapi import BackgroundTasks
import json
import math
import random

@router.post("/generate-world")
async def generate_world(prompt: str, size_km: float, density: str, background_tasks: BackgroundTasks):
    """
    Generate a procedural world map based on prompt.
    Returns a JSON with chunk data (object placements).
    """
    # Hitung jumlah objek berdasarkan densitas
    density_multiplier = {"low": 0.5, "medium": 1.0, "high": 2.0, "ultra": 4.0}.get(density, 1.0)
    base_objects_per_km2 = 500  # pohon, batu, rumput
    total_objects = int(size_km * size_km * base_objects_per_km2 * density_multiplier)

    chunks = {}
    chunk_size = 256  # meter
    chunks_per_km = 1000 / chunk_size

    # Seed berdasarkan prompt untuk reproduktifitas
    seed = sum(ord(c) for c in prompt)
    random.seed(seed)

    # Generate biomes dari prompt (LLM bisa digunakan di sini)
    biomes = ["forest", "plains", "hills", "desert"]
    if "medieval" in prompt.lower():
        biomes = ["forest", "village", "castle", "farmland"]

    world_data = {
        "seed": seed,
        "size_km": size_km,
        "chunk_size": chunk_size,
        "biomes": biomes,
        "objects": []
    }

    # Generate penempatan objek
    for _ in range(total_objects):
        x = random.uniform(-size_km * 500, size_km * 500)
        z = random.uniform(-size_km * 500, size_km * 500)
        obj_type = random.choice(["tree_oak", "tree_pine", "rock", "bush", "flower"])
        if random.random() < 0.01:  # 1% bangunan
            obj_type = random.choice(["house_wood", "tower_stone", "windmill"])
        world_data["objects"].append({
            "type": obj_type,
            "x": x, "y": 0, "z": z,
            "scale": random.uniform(0.8, 1.2)
        })

    # Simpan ke file sementara (bisa di-cache)
    output_dir = "/sdcard/NeoEngine/Worlds"
    os.makedirs(output_dir, exist_ok=True)
    filename = f"world_{seed}_{size_km}km.json"
    filepath = os.path.join(output_dir, filename)
    with open(filepath, 'w') as f:
        json.dump(world_data, f)

    return {
        "status": "generated",
        "file": filepath,
        "total_objects": total_objects,
        "chunks": len(chunks)
    }

@router.post("/generate-massive-world")
async def generate_massive_world(request: Request):
    data = await request.json()
    theme = data.get('theme', 'fantasy')
    size_km = data.get('sizeKm', 100)
    # Panggil C++ executable atau gunakan Python binding untuk precompute
    # Untuk sekarang, return seed dan config
    import random
    seed = random.randint(1, 2**31-1)
    return {
        "seed": seed,
        "sizeKm": size_km,
        "chunkSize": 256,
        "status": "ready"
    }
