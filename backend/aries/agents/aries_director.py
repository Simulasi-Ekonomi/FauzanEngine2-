"""
ARIES Director + HERMES + RUFLO
Pakai urllib (sync) via run_in_executor - terbukti jalan di Termux
"""

import asyncio
import json
import os
import ssl
import urllib.request
from typing import AsyncGenerator

GITHUB_API_URL = "https://models.inference.ai.azure.com/chat/completions"
GITHUB_TOKEN   = os.environ.get("GITHUB_TOKEN", "")

SSL_CTX = ssl.create_default_context()
SSL_CTX.check_hostname = False
SSL_CTX.verify_mode    = ssl.CERT_NONE

ARIES_SYSTEM = """Kamu adalah ARIES, Game Director AI dari FauzanEngine buatan Fauzan.
MISI: Membuat game lengkap secara autonomous dari satu prompt.
Koordinasi HERMES (narrative) dan RUFLO (world building).
Output JSON siap pakai untuk NeoEngine. Bahasa Indonesia. Proaktif."""

HERMES_SYSTEM = """Kamu adalah HERMES, Narrative Agent FauzanEngine.
Spesialisasi: lore, dialog NPC, quest, item description, fraksi.
Output JSON siap masuk NeoEngine. Bahasa Indonesia."""

RUFLO_SYSTEM = """Kamu adalah RUFLO, World Builder Agent FauzanEngine.
Spesialisasi: world map, biome, dungeon, kota, spawn table, ekonomi zone.
Output JSON WorldData siap masuk NeoEngine."""


def _call_api_sync(messages: list, model: str = "gpt-4o-mini", max_tokens: int = 2048) -> str:
    """Sync API call pakai urllib - terbukti jalan di Termux"""
    if not GITHUB_TOKEN:
        return "ERROR: GITHUB_TOKEN tidak di-set"

    payload = json.dumps({
        "model": model,
        "messages": messages,
        "stream": False,
        "temperature": 0.7,
        "max_tokens": max_tokens
    }).encode("utf-8")

    req = urllib.request.Request(
        GITHUB_API_URL,
        data=payload,
        headers={
            "Authorization": f"Bearer {GITHUB_TOKEN}",
            "Content-Type": "application/json"
        }
    )

    try:
        with urllib.request.urlopen(req, context=SSL_CTX, timeout=120) as resp:
            data = json.loads(resp.read())
            return data["choices"][0]["message"]["content"]
    except Exception as e:
        return f"[Error: {e}]"


class GameAgent:
    def __init__(self, name: str, system: str, model: str = "gpt-4o-mini"):
        self.name    = name
        self.system  = system
        self.model   = model
        self.history = []

    async def chat(self, message: str) -> AsyncGenerator[str, None]:
        """Async wrapper - jalankan sync call di thread pool"""
        self.history.append({"role": "user", "content": message})

        messages = [
            {"role": "system", "content": self.system},
            *self.history[-10:]
        ]

        loop   = asyncio.get_event_loop()
        result = await loop.run_in_executor(
            None, _call_api_sync, messages, self.model, 2048
        )

        self.history.append({"role": "assistant", "content": result})

        # Simulasi streaming - kirim per kata
        for word in result.split(" "):
            yield word + " "
            await asyncio.sleep(0.01)

    async def chat_full(self, message: str) -> str:
        self.history.append({"role": "user", "content": message})
        messages = [
            {"role": "system", "content": self.system},
            *self.history[-10:]
        ]
        loop   = asyncio.get_event_loop()
        result = await loop.run_in_executor(
            None, _call_api_sync, messages, self.model, 2048
        )
        self.history.append({"role": "assistant", "content": result})
        return result

    async def chat_json(self, message: str) -> dict:
        full = await self.chat_full(message)
        return self._parse_json(full) or {"raw": full}

    def _parse_json(self, text: str) -> dict:
        try:
            if "```json" in text:
                s = text.find("```json") + 7
                e = text.find("```", s)
                return json.loads(text[s:e].strip())
            elif "{" in text:
                s = text.find("{")
                e = text.rfind("}") + 1
                return json.loads(text[s:e])
        except:
            pass
        return {}

    def reset(self):
        self.history = []


class AriesDirector:
    def __init__(self):
        self.aries  = GameAgent("Aries",  ARIES_SYSTEM,  "gpt-4o-mini")
        self.hermes = GameAgent("Hermes", HERMES_SYSTEM, "gpt-4o-mini")
        self.ruflo  = GameAgent("Ruflo",  RUFLO_SYSTEM,  "gpt-4o-mini")
        self.gdd        = {}
        self.task_queue = []
        self.completed  = []
        self.game_data  = {}

    async def chat(self, message: str) -> AsyncGenerator[str, None]:
        async for token in self.aries.chat(message):
            yield token

    async def create_game(self, prompt: str) -> AsyncGenerator[dict, None]:
        yield {"phase": "planning", "message": f"Aries menerima: {prompt}"}
        yield {"phase": "gdd", "message": "Membuat Game Design Document..."}

        gdd_prompt = f"""Buat GDD untuk game: "{prompt}"

Format JSON:
{{
  "title": "nama game",
  "genre": "genre",
  "setting": "setting dunia",
  "world": {{"size_km": 10, "biomes": ["list"], "lore_summary": "ringkasan"}},
  "player": {{"classes": ["list"], "progression": "sistem level", "stats": ["list"]}},
  "systems": {{"combat": "desc", "economy": "desc", "quest": "desc"}},
  "content": {{"zones": 5, "levels": 10, "items": 50, "enemies": 20}},
  "monetization": {{"model": "free to play", "iap": ["list"]}}
}}"""

        gdd_text = await self.aries.chat_full(gdd_prompt)
        self.gdd = self.aries._parse_json(gdd_text) or {"title": prompt, "raw": gdd_text}

        # Stream GDD sebagai tokens
        for word in gdd_text[:500].split(" "):
            yield {"phase": "gdd", "token": word + " "}
            await asyncio.sleep(0.005)

        yield {"phase": "gdd", "message": "GDD selesai!", "data": self.gdd}

        # Task queue
        self.task_queue = [
            {"id": "world",      "agent": "ruflo",  "type": "world"},
            {"id": "levels",     "agent": "ruflo",  "type": "levels"},
            {"id": "lore",       "agent": "hermes", "type": "lore"},
            {"id": "quests",     "agent": "hermes", "type": "quests"},
            {"id": "npcs",       "agent": "hermes", "type": "npcs"},
            {"id": "items",      "agent": "aries",  "type": "items"},
            {"id": "enemies",    "agent": "aries",  "type": "enemies"},
            {"id": "combat",     "agent": "aries",  "type": "combat"},
        ]

        yield {"phase": "tasks", "message": f"{len(self.task_queue)} tasks dibuat",
               "data": [t["id"] for t in self.task_queue]}

        ctx = json.dumps(self.gdd, ensure_ascii=False)[:1000]
        prompts = {
            "world":   f"GDD:\n{ctx}\n\nGenerate world/map data JSON untuk game ini.",
            "levels":  f"GDD:\n{ctx}\n\nGenerate 10 level dengan deskripsi, musuh, boss dalam JSON.",
            "lore":    f"GDD:\n{ctx}\n\nGenerate lore: sejarah, fraksi, mitologi dalam JSON.",
            "quests":  f"GDD:\n{ctx}\n\nGenerate 5 quest utama dengan objectives dan rewards JSON.",
            "npcs":    f"GDD:\n{ctx}\n\nGenerate 10 NPC dengan nama, peran, dialog JSON.",
            "items":   f"GDD:\n{ctx}\n\nGenerate 20 item dengan nama, tipe, stat JSON.",
            "enemies": f"GDD:\n{ctx}\n\nGenerate 15 musuh dengan stat, drop, AI behavior JSON.",
            "combat":  f"GDD:\n{ctx}\n\nGenerate combat system config JSON.",
        }

        agents = {"aries": self.aries, "hermes": self.hermes, "ruflo": self.ruflo}

        for i, task in enumerate(self.task_queue):
            agent = agents[task["agent"]]
            yield {
                "phase": "execution",
                "task": task["id"],
                "agent": task["agent"],
                "message": f"[{task['agent'].upper()}] Generating {task['id']}...",
                "progress": f"{i+1}/{len(self.task_queue)}"
            }

            result = await agent.chat_json(prompts[task["type"]])
            self.game_data[task["id"]] = result
            self.completed.append(task["id"])

            yield {
                "phase": "execution",
                "task": task["id"],
                "agent": task["agent"],
                "message": f"✓ {task['id']} selesai"
            }

        final = {
            "gdd":      self.gdd,
            "world":    {k: self.game_data.get(k, {}) for k in ["world", "levels"]},
            "narrative":{k: self.game_data.get(k, {}) for k in ["lore", "quests", "npcs"]},
            "content":  {k: self.game_data.get(k, {}) for k in ["items", "enemies"]},
            "systems":  {"combat": self.game_data.get("combat", {})},
            "meta":     {"tasks_completed": len(self.completed), "engine": "FauzanEngine v2.0"}
        }

        yield {"phase": "done", "message": "🎮 Game berhasil dibuat!", "data": final}

    def reset(self):
        for a in [self.aries, self.hermes, self.ruflo]:
            a.reset()
        self.gdd={}; self.task_queue=[]; self.completed=[]; self.game_data={}
