"""
WebSocket Bridge - Real-time communication between Editor, Aries AI, and Engine Runtime.
"""

import json
from fastapi import WebSocket


class ConnectionManager:
    """Manages WebSocket connections between editor clients and engine runtime."""

    def __init__(self):
        self.active_connections: list[WebSocket] = []
        self.engine_connection: WebSocket | None = None

    async def connect(self, websocket: WebSocket):
        """Accept a new WebSocket connection."""
        await websocket.accept()
        self.active_connections.append(websocket)
        print(f"[WS] Client connected. Total: {len(self.active_connections)}")

    def disconnect(self, websocket: WebSocket):
        """Remove a disconnected WebSocket."""
        if websocket in self.active_connections:
            self.active_connections.remove(websocket)
        if websocket == self.engine_connection:
            self.engine_connection = None
        print(f"[WS] Client disconnected. Total: {len(self.active_connections)}")

    async def broadcast(self, message: str, exclude: WebSocket | None = None):
        """Broadcast a message to all connected clients."""
        disconnected = []
        for connection in list(self.active_connections):
            if connection == exclude:
                continue
            try:
                await connection.send_text(message)
            except Exception:
                disconnected.append(connection)
        
        for conn in disconnected:
            self.disconnect(conn)

    async def send_to_engine(self, message: dict):
        """Send a message specifically to the engine runtime."""
        if self.engine_connection:
            try:
                await self.engine_connection.send_json(message)
            except Exception:
                self.engine_connection = None

    def active_count(self) -> int:
        """Get the number of active connections."""
        return len(self.active_connections)

    def register_engine(self, websocket: WebSocket):
        """Register a connection as the engine runtime."""
        self.engine_connection = websocket
        print("[WS] Engine runtime connected")
