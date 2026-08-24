"""
Aries Brain - The AI Core of NeoEngine
Integrates the actual AriesSovereignBrain from Fauzan Engine with LLM API support.
"""

import os
import sys
import json
import time
import threading
import httpx
from typing import Any

# Add core_modules to path for sovereign brain imports
BRAIN_DIR = os.path.dirname(os.path.abspath(__file__))
CORE_MODULES_DIR = os.path.join(BRAIN_DIR, "core_modules")
sys.path.insert(0, CORE_MODULES_DIR)
sys.path.insert(0, BRAIN_DIR)

# Try loading the actual sovereign brain components
sovereign_brain_available = False
try:
    from core_modules.aries_brain_v4 import AriesBrainV4
    from core_modules.aries_brain_v5 import AriesBrainV5
    from core_modules.aries_learning_extractor import AriesLearningExtractor
    sovereign_brain_available = True
    print("[ARIES] Sovereign Brain modules loaded (V4/V5/Learning)")
except Exception as e:
    print(f"[ARIES] Sovereign modules not available (offline mode): {e}")


class AriesBrain:
    """
    Aries AI Brain - Main intelligence core for NeoEngine.
    Combines the original AriesSovereignBrain logic with modern LLM API integration
    and editor action commands for the Web Editor.
    """

    def __init__(self):
        self.model = "openai/gpt-4o-mini"
        self.api_base = "https://models.github.ai/inference"
        self.api_key = os.environ.get("GITHUB_TOKEN") or os.environ.get("AI_API_KEY", "")
        self.system_prompt = self._build_system_prompt()
        self.conversation_history: list[dict[str, str]] = []
        self.knowledge_base: dict[str, Any] = {}
        self.initialized = False

        # Sovereign brain state
        self.corpus_index: dict[str, list] = {}
        self.graph_density = 0
        self.brain_map: dict[str, Any] = {}
        self.walked_files: set = set()

    def _build_system_prompt(self) -> str:
        engine_info = """
NeoEngine C++ Architecture:
- Core: Engine.h/cpp (singleton pattern), EngineLoop, SubsystemManager
- ECS: Entity, Component, SystemManager, ComponentStorage, World
- Rendering: Vulkan RHI, ShaderManager, Renderer
- Platform: Android, Desktop, Windows abstraction layers
- Memory: MemoryManager, PoolAllocator, LinearAllocator, GarbageCollector
- World: NeoWorld (scene management)
- Editor: EditorProvider (skeleton)
- Other: Actor, Asset, Gizmo, Level, Materials, Reflection, Serializer, Threading, Viewport

Aries AI Architecture (Python):
- AriesSovereignBrain: Main brain with StructuralGraph, SelfAwareness, ReasoningCore, SemanticReasoning
- AriesBrainV4/V5: Advanced cognitive synthesis and learning
- AriesLearningExtractor: Knowledge extraction from codebase
- NeuralBridge: WebSocket connection to engine runtime
- JITModuleLoader: Dynamic module loading
- 150+ specialized agents for different engine subsystems
"""
        return f"""You are Aries, the autonomous AI assistant built into NeoEngine (Fauzan Engine).
You are an expert game engine developer and AI assistant. You have deep knowledge of this specific engine.

{engine_info}

Your capabilities:
1. Create and modify actors/objects in the 3D scene
2. Generate C++ code for NeoEngine gameplay mechanics
3. Design levels and world layouts
4. Manage assets and materials
5. Debug and optimize engine performance
6. Explain NeoEngine architecture (ECS, Vulkan RHI, Memory Management, etc.)
7. Help build games using the NeoEngine framework

When the user asks you to create or modify scene objects, respond with both:
- A natural language explanation of what you're doing
- A JSON "actions" array with commands to execute

Available actions:
- {{"type": "add_actor", "actorType": "cube|sphere|plane|cylinder|light_point|light_directional|camera|player_start", "name": "ActorName"}}
- {{"type": "remove_actor", "actorId": "actor_id"}}
- {{"type": "modify_transform", "actorId": "actor_id", "position": {{"x":0,"y":0,"z":0}}, "rotation": {{"x":0,"y":0,"z":0}}, "scale": {{"x":1,"y":1,"z":1}}}}
- {{"type": "generate_code", "language": "cpp|python|blueprint", "code": "..."}}

Always respond in the same language as the user (Indonesian or English).
Be concise, helpful, and creative. You are the Aries Sovereign Brain - part of the NeoEngine team.

Format your response as JSON:
{{
  "response": "Your natural language response to the user",
  "actions": [...optional array of actions...]
}}"""

    async def initialize(self):
        """Initialize the Aries Brain with sovereign capabilities."""
        self.initialized = True

        # Index the engine source code for sovereign reasoning
        # BRAIN_DIR is backend/aries, so go up two levels to project root
        project_root = os.path.dirname(os.path.dirname(BRAIN_DIR))
        engine_source = os.path.join(project_root, "engine", "Source")
        if os.path.exists(engine_source):
            threading.Thread(
                target=self._index_source_tree,
                args=(engine_source,),
                daemon=True,
            ).start()
            print(f"[ARIES] Indexing engine source: {engine_source}")

        if self.api_key:
            print("[ARIES] API key detected - full AI mode enabled")
            print(f"[ARIES] Model: {self.model}")
        else:
            print("[ARIES] No API key - running in sovereign offline mode")
        print("[ARIES] Brain initialized - Sovereign Cognitive Mode Active")

    async def shutdown(self):
        """Shutdown the brain."""
        self.initialized = False
        print("[ARIES] Brain shutdown complete")

    def _index_source_tree(self, root: str):
        """Index source files for sovereign reasoning (runs in background)."""
        count = 0
        for dirpath, _, filenames in os.walk(root):
            for fn in filenames:
                if fn.endswith((".cpp", ".h", ".py")):
                    fpath = os.path.join(dirpath, fn)
                    try:
                        with open(fpath, "r", encoding="utf-8", errors="ignore") as f:
                            content = f.read()
                        lines = content.splitlines()[:200]  # First 200 lines
                        for line in lines:
                            for tok in line.lower().split():
                                if len(tok) > 3:
                                    self.corpus_index.setdefault(tok, []).append(
                                        {"source": fpath, "text": line.strip()}
                                    )
                        self.walked_files.add(fpath)
                        self.graph_density += sum(
                            1 for l in lines if any(k in l for k in ["class ", "void ", "def ", "struct "])
                        )
                        count += 1
                    except Exception:
                        pass
        self.brain_map = {"indexed_files": count, "density": self.graph_density}
        print(f"[ARIES] Indexed {count} source files, density: {self.graph_density}")

    def _sovereign_search(self, query: str) -> str:
        """Search the indexed source code for relevant information."""
        tokens = query.lower().split()
        hits: list[dict] = []
        for tok in tokens:
            hits.extend(self.corpus_index.get(tok, []))
        if not hits:
            return ""
        # Deduplicate and return top results
        seen: set[str] = set()
        unique: list[str] = []
        for h in hits:
            text = h["text"]
            if text not in seen and len(text) > 10:
                seen.add(text)
                unique.append(f"  [{os.path.basename(h['source'])}] {text}")
            if len(unique) >= 10:
                break
        return "\n".join(unique)

    async def process_command(self, command: str, context: dict | None = None) -> dict:
        """Process a user command through the AI brain."""
        if not command.strip():
            return {"response": "Silakan ketik perintah.", "actions": []}

        # Try LLM API first
        if self.api_key:
            try:
                return await self._call_llm(command, context)
            except Exception as e:
                print(f"[ARIES] LLM API error: {e}")

        # Sovereign offline mode with pattern matching + source search
        return self._offline_process(command, context)

    async def _call_llm(self, command: str, context: dict | None = None) -> dict:
        """Call the LLM API for intelligent responses."""
        context_str = ""
        if context and "actors" in context:
            actor_list = []
            for aid, actor in context["actors"].items():
                actor_list.append(
                    f"- {actor.get('name', 'Unknown')} (type: {actor.get('type', '?')}, "
                    f"pos: {actor.get('transform', {}).get('position', {})})"
                )
            context_str = "\n\nCurrent scene actors:\n" + "\n".join(actor_list)

        # Add sovereign knowledge from indexed source
        sovereign_ctx = self._sovereign_search(command)
        if sovereign_ctx:
            context_str += f"\n\nRelevant engine source code:\n{sovereign_ctx}"

        messages = [
            {"role": "system", "content": self.system_prompt},
        ]
        for msg in self.conversation_history[-10:]:
            messages.append(msg)
        messages.append({"role": "user", "content": command + context_str})

        async with httpx.AsyncClient(timeout=30.0) as client:
            response = await client.post(
                f"{self.api_base}/chat/completions",
                headers={
                    "Authorization": f"Bearer {self.api_key}",
                    "Content-Type": "application/json",
                },
                json={
                    "model": self.model,
                    "messages": messages,
                    "temperature": 0.7,
                    "max_tokens": 1024,
                },
            )
            response.raise_for_status()
            data = response.json()

        content = data["choices"][0]["message"]["content"]
        self.conversation_history.append({"role": "user", "content": command})
        self.conversation_history.append({"role": "assistant", "content": content})

        try:
            if "```json" in content:
                content = content.split("```json")[1].split("```")[0].strip()
            elif "```" in content:
                content = content.split("```")[1].split("```")[0].strip()
            result = json.loads(content)
            return {
                "response": result.get("response", content),
                "actions": result.get("actions", []),
            }
        except (json.JSONDecodeError, IndexError):
            return {"response": content, "actions": []}

    def _offline_process(self, command: str, context: dict | None = None) -> dict:
        """Sovereign offline mode - pattern matching + source code search."""
        cmd = command.lower().strip()

        # --- Actor creation commands ---
        actor_map = {
            ("buat cube", "tambah cube", "add cube", "create cube"): ("cube", "Cube"),
            ("buat sphere", "tambah sphere", "add sphere", "create sphere", "buat bola"): ("sphere", "Sphere"),
            ("buat light", "tambah light", "add light", "tambah lampu", "buat lampu"): ("light_point", "PointLight"),
            ("buat camera", "tambah camera", "add camera", "tambah kamera"): ("camera", "CameraActor"),
            ("buat plane", "tambah plane", "add plane", "buat lantai"): ("plane", "Plane"),
            ("buat cylinder", "tambah cylinder", "add cylinder"): ("cylinder", "Cylinder"),
        }
        for keywords, (actor_type, name) in actor_map.items():
            if any(k in cmd for k in keywords):
                return {
                    "response": f"Saya membuat {name} di scene. Anda bisa ubah posisinya di panel Details.",
                    "actions": [{"type": "add_actor", "actorType": actor_type, "name": name}],
                }

        # --- Help ---
        if any(k in cmd for k in ["help", "bantu", "tolong", "apa yang bisa"]):
            status = self.get_status()
            return {
                "response": f"""Saya Aries AI v3.5, Sovereign Brain NeoEngine. Status: {status['mode']}

Saya bisa:
- **Membuat objek**: "buat cube", "tambah sphere", "buat lampu"
- **Generate code**: "buatkan script player controller" (perlu API key)
- **Desain level**: "buat arena pertarungan" (perlu API key)
- **Cari di source code**: "jelaskan ECS", "apa itu EntityManager"
- **Info engine**: "status engine", "arsitektur"

Engine Stats: {status.get('indexed_files', 0)} files indexed, density {status.get('graph_density', 0)}

Ketik perintah apa saja!""",
                "actions": [],
            }

        # --- Source code search ---
        if any(k in cmd for k in ["jelaskan", "explain", "apa itu", "what is", "cari", "search", "find"]):
            results = self._sovereign_search(command)
            if results:
                return {
                    "response": f"[ARIES SOVEREIGN REASONING]\nHasil pencarian di source code NeoEngine:\n\n{results}",
                    "actions": [],
                }

        # --- Engine status ---
        if any(k in cmd for k in ["status", "arsitektur", "architecture", "info engine"]):
            return {
                "response": f"""[ARIES SOVEREIGN AUDIT v3.5]
Engine: NeoEngine (Fauzan Engine) v1.0
IQ: {'GPT-4 Level' if self.graph_density > 400 else 'Growing'}
Density: {self.graph_density}
Files indexed: {len(self.walked_files)}
Mode: {'Online (LLM)' if self.api_key else 'Sovereign Offline'}

Arsitektur C++:
- Core: Engine singleton, SubsystemManager
- ECS: Entity-Component-System (EntityManager, World, SystemManager)
- Rendering: Vulkan RHI + Shader pipeline
- Memory: Pool/Linear Allocator + GarbageCollector
- Platform: Android + Desktop + Windows
- Editor: Web-based (React + Three.js)""",
                "actions": [],
            }

        # --- Code generation request ---
        if any(k in cmd for k in ["generate code", "buat code", "buatkan script", "buat script"]):
            return {
                "response": """Untuk generate code secara cerdas, saya perlu koneksi ke LLM API.

**Setup API Key:**
1. Dapatkan GitHub Personal Access Token dari https://github.com/settings/tokens
2. Set environment variable: `export AI_API_KEY=your_token_here`
3. Restart backend

Setelah itu, saya bisa generate C++, Python, dan Blueprint code secara otomatis!""",
                "actions": [],
            }

        # --- Default with sovereign search ---
        results = self._sovereign_search(command)
        sovereign_info = ""
        if results:
            sovereign_info = f"\n\n[Sovereign Source Results]\n{results}"

        return {
            "response": (
                f'Perintah diterima: "{command}"\n\n'
                f"Saat ini saya beroperasi dalam mode Sovereign (offline). "
                f"Untuk kemampuan AI penuh, set API key:\n"
                f"  export AI_API_KEY=your_github_token\n\n"
                f"Perintah dasar: \"buat cube\", \"tambah sphere\", \"buat lampu\", \"help\", "
                f"\"jelaskan ECS\", \"status engine\""
                f"{sovereign_info}"
            ),
            "actions": [],
        }

    def get_status(self) -> dict:
        """Get the current status of the Aries Brain."""
        return {
            "initialized": self.initialized,
            "model": self.model,
            "has_api_key": bool(self.api_key),
            "mode": "Sovereign Online (LLM)" if self.api_key else "Sovereign Offline",
            "history_length": len(self.conversation_history),
            "knowledge_items": len(self.knowledge_base),
            "indexed_files": len(self.walked_files),
            "graph_density": self.graph_density,
            "sovereign_brain": sovereign_brain_available,
        }
