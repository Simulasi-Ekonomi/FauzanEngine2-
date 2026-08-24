import os
import sys
import time
import threading
import traceback
import json
import uvicorn
import asyncio
import fastapi.routing
from fastapi import FastAPI, Request
from fastapi.responses import JSONResponse

# =========================================================
# 🛡️ PHASE 1: DEEP HOOKING (UTUH)
# =========================================================

if not hasattr(fastapi.routing.APIRoute, "_aries_patched"):
    _original_init = fastapi.routing.APIRoute.__init__
    _orig_run = fastapi.routing.run_endpoint_function

    def _master_patched_init(self, *a, **kw):
        _original_init(self, *a, **kw)
        if getattr(self, "path", None) == "/interact":
            original_call = self.dependant.call
            async def safe_call(*args, **kwargs):
                if "data" in kwargs:
                    d = kwargs["data"]
                    if isinstance(d, dict) and "prompt" in d:
                        d["prompt"] = str(d["prompt"])
                return await original_call(*args, **kwargs)
            self.dependant.call = safe_call

    async def master_run_endpoint_function(*args, **kwargs):
        values = kwargs.get("values") or (args[2] if len(args) > 2 else None)
        if isinstance(values, dict) and "data" in values:
            d = values["data"]
            if isinstance(d, dict) and "prompt" in d:
                d["prompt"] = str(d["prompt"])
        return await _orig_run(*args, **kwargs)

    fastapi.routing.APIRoute.__init__ = _master_patched_init
    fastapi.routing.run_endpoint_function = master_run_endpoint_function
    fastapi.routing.APIRoute._aries_patched = True
    print("🛡️ PreRoute & Runtime Shield Active (Hardened)")

# =========================================================
# ⚡ PHASE 1.5: STANDBY AGENT FACTORY (FRAGMENTED)
# =========================================================

class AgentFactory:
    _loaded_agents = {}
    _last_access = {}
    _lock = asyncio.Lock()

    @classmethod
    async def get_agent(cls, agent_name):
        async with cls._lock:
            if agent_name not in cls._loaded_agents:
                print(f"⚡ [STANDBY-MODE] Waking up Agent: {agent_name}")
                try:
                    module = __import__(agent_name)
                    agent_class = getattr(module, agent_name)
                    cls._loaded_agents[agent_name] = agent_class()
                except Exception as e:
                    print(f"❌ Error Loading {agent_name}: {e}")
                    return None
            cls._last_access[agent_name] = time.time()
            return cls._loaded_agents[agent_name]

    @classmethod
    async def cleanup_idle(cls, timeout=300):
        while True:
            now = time.time()
            to_remove = []
            async with cls._lock:
                for name, last_time in cls._last_access.items():
                    if now - last_time > timeout:
                        to_remove.append(name)
                for name in to_remove:
                    print(f"🛑 [HIBERNATE] Agent {name} Standby (RAM Cleared).")
                    if name in cls._loaded_agents: del cls._loaded_agents[name]
                    if name in cls._last_access: del cls._last_access[name]
            await asyncio.sleep(60)

# =========================================================
# ⚡ PHASE 2 & 3: CORE BRAIN LOAD
# =========================================================

try:
    import aries_perfect as aries_core
    from aries_perfect import app, brain
    import aries_auto_injector
    aries_auto_injector.inject_to_aries(brain)
    print("✅ Enhanced ML Brain + Auto Injector Active")
except Exception as e:
    print(f"❌ Critical Error: Failed loading aries_perfect: {e}")
    sys.exit(1)

# =========================================================
# 📊 PHASE 4-7: OBSERVABILITY & MIDDLEWARE (UTUH)
# =========================================================

@app.on_event("startup")
async def startup_event():
    asyncio.create_task(AgentFactory.cleanup_idle())

@app.get("/brain_status")
def brain_status():
    return {
        "status": "ONLINE",
        "mode": "Enhanced ML Standby Active",
        "active_agents": list(AgentFactory._loaded_agents.keys()),
        "logic_density": getattr(brain.graph, 'total_logic_density', 0),
        "pid": os.getpid()
    }

@app.post("/semantic_query")
async def semantic_query(request: Request):
    data = await request.json()
    return {"response": brain.semantic.generate_response(data.get("prompt",""), brain.reasoning)}

@app.post("/execute_agent")
async def execute_agent(request: Request):
    data = await request.json()
    target = data.get("agent")
    agent = await AgentFactory.get_agent(target)
    if agent:
        result = agent.run() if hasattr(agent, 'run') else "Agent Active"
        return {"status": "success", "result": result}
    return {"status": "error", "message": "Agent not found"}

@app.middleware("http")
async def sanitize_middleware(request: Request, call_next):
    if request.url.path == "/interact" and request.method == "POST":
        body = await request.body()
        try:
            data = json.loads(body)
            if isinstance(data, dict) and "prompt" in data:
                data["prompt"] = str(data["prompt"])
            request._json = data
            async def receive(): return {"type":"http.request","body":body}
            request._receive = receive
        except Exception as e: print(f'Logging: {e}')
    return await call_next(request)

if __name__ == "__main__":
    print(f"🚀 Aries Enhanced Server Hardened PID {os.getpid()}")
    uvicorn.run(app, host="0.0.0.0", port=3333, reload=False, access_log=False, log_level="warning")
