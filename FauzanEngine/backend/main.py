"""
FauzanEngine Backend
FastAPI server - Aries Brain, Game Director, WebSocket bridge
"""

import os
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

from api.routes import router as api_router
from aries.game_director.routes import router as director_router

app = FastAPI(
    title="FauzanEngine Backend",
    description="AI-powered game engine backend dengan Aries, Hermes, dan Ruflo",
    version="2.0.0"
)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

app.include_router(api_router)
app.include_router(director_router)

@app.get("/")
async def root():
    return {
        "name": "FauzanEngine",
        "version": "2.0.0",
        "agents": ["Aries (Director)", "Hermes (Narrative)", "Ruflo (World Builder)"],
        "github_token": "configured" if os.environ.get("GITHUB_TOKEN") else "NOT SET"
    }

@app.get("/health")
async def health():
    return {"status": "ok"}

if __name__ == "__main__":
    import uvicorn
    port = int(os.environ.get("PORT", 8000))
    uvicorn.run(app, host="0.0.0.0", port=port, reload=False)
