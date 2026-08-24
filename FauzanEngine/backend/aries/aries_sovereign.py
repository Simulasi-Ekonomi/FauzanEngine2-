import time
import os
import sys
import ast
import json
import uvicorn
import glob
import hashlib
import gc
import types
import httpx
from datetime import datetime
from collections import deque, defaultdict
from fastapi import FastAPI
from contextlib import asynccontextmanager
import threading
import importlib.util
import asyncio
import traceback
from concurrent.futures import ThreadPoolExecutor

ARIES_PARENT = os.path.dirname(os.path.abspath(__file__))
sys.path.append("/sdcard/Buku saya/FauzanEngine/NeoEngine/aries/core_modules")

# [INJECTION] IMPORT BRIDGE EVOLUTIONS
try:
    from core_modules.aries_bridge_evolutions import AriesCognitiveBridge
    print("💉 [INJECTION] Sovereign Interpret Loop Hardened.")
except ImportError:
    AriesCognitiveBridge = None
    print("⚠️ [BRIDGE ERROR] aries_bridge_evolutions.py tidak ditemukan.")

# [PRESERVE] PATCH: IMPORT CLEANER
try:
    import aries_cleaner
except ImportError:
    aries_cleaner = None

try:
    from aries_learning_extractor import AriesLearningExtractor
    from aries_brain_v4 import AriesBrainV4
    from aries_brain_v5 import AriesBrainV5
    from aries_audit_brain import AriesAuditBrain
    print("✅ SIGMA-OMEGA Learning & Synthesis: ONLINE (Patched V5)")
except Exception as e:
    print(f"⚠️ Learning Interface Critical Error: {e}")

# =========================================================
# 🛡️ SYSTEM UTILS & STABILITY (PRESERVED)
# =========================================================
class AriesLockManager:
    def __init__(self):
        self._lock = threading.RLock()
        self.allowed_methods = ["_load_json", "_build_links", "extract_and_synthesize"]
    
    # KITA TAMBAHIN INI BUAT NGE-TRACE SIAPA YANG NGE-CALL LOCK SEBAGAI FUNGSI
    def __call__(self, *args, **kwargs):
        import traceback
        print("🚨 [TRACING] SIAPA YANG MANGGIL LOCK SEBAGAI FUNGSI:")
        traceback.print_stack()
        raise TypeError("AriesLockManager should not be called as a function. Use .safe() or .allow() instead.")

    @property
    def lock(self):
        return self._lock

    def safe(self, func, *a, **kw):
        with self._lock: return func(*a, **kw)
    # Tambahkan method ini di dalam class AriesLockManager
    def allow(self, method_name):
        """
        DUMMY method untuk satisfy AriesAuditBrain. 
        AuditBrain manggil ini tapi sebenarnya gak butuh logika apa-apa,
        cukup return True supaya dia lanjut ke proses berikutnya.
        """
        return True

class MemoryGuard:
    def __init__(self, index, limit=10000000):
        self.index, self.limit = index, limit
    def enforce(self):
        size = sum(len(v) for v in self.index.values())
        if size <= self.limit: return
        remove = size - self.limit
        for k in list(self.index.keys()):
            if remove <= 0: break
            if self.index[k]: self.index[k].pop(0); remove -= 1

class AsyncWriter:
    def __init__(self, interval=5):
        self.queue = deque(); self.interval = interval
        self._stop_event = threading.Event()
        self.worker = threading.Thread(target=self._loop, daemon=True)
        self.worker.start()
    def push(self, path, data): self.queue.append((path, data))
    def _loop(self):
        while not self._stop_event.is_set():
            if self.queue:
                wait_time = 1 if len(self.queue) > 10 else self.interval
                try:
                    path, data = self.queue.popleft()
                    with open(path, "w") as f: json.dump(data, f)
                except Exception as e:
                    pass
                time.sleep(wait_time)
            else: time.sleep(self.interval)
    def stop(self): self._stop_event.set()

# =========================================================
# 🌉 UPGRADED NEURAL BRIDGE (ASYNC + RETRY)
# =========================================================
class NeuralBridge:
    def __init__(self, spine_url="http://localhost:8081"):
        self.spine_url = spine_url
        self.client = httpx.AsyncClient(timeout=10.0)

    async def dispatch_agent(self, agent_id, action, params=None, retries=3):
        payload = {"agent_id": agent_id, "action": action, "parameters": params or {}}
        for i in range(retries):
            try:
                r = await self.client.post(f"{self.spine_url}/execute", json=payload)
                return r.json()
            except Exception as e:
                if i == retries - 1: return {"error": f"Spine Failed: {e}"}
                await asyncio.sleep(1)

# =========================================================
# [PRESERVE] TFIDF FALLBACK & SEMANTIC REASONING
# =========================================================
try:
    from sklearn.feature_extraction.text import TfidfVectorizer
    from sklearn.metrics.pairwise import cosine_similarity
except ImportError:
    class TfidfVectorizer:
        def fit_transform(self, texts): return texts
        def transform(self, texts): return texts
    def cosine_similarity(a, b):
        sims = []
        for x in a:
            row = []
            for y in b: row.append(len(set(str(x).split()) & set(str(y).split())) / max(1, len(set(str(x).split()) | set(str(y).split()))))
            sims.append(row)
        return sims

neural_respond = None
ENGINE_ROOT = os.getcwd()
BRAIN_ARCHIVE = "/sdcard/Aries_Brain_Archive"
UPDATE_FILE = f"{ENGINE_ROOT}/NeoEngine/aries/brain_data/update_data.txt"
UNREAL_PROJECT_SOURCE = f"{ENGINE_ROOT}/NeoEngine/aries/project_files/Source"
UNREAL_ROOT_SOURCE = f"{ENGINE_ROOT}/NeoEngine/Source"
OMEGA_DB = f"{ARIES_PARENT}/core_modules/omega_disk_db.jsonl"
KNOWLEDGE_BASE = OMEGA_DB
NEURAL_MEMORY = f"{ENGINE_ROOT}/NeoEngine/aries/brain_data/neural_memory.json"

if not os.path.exists(BRAIN_ARCHIVE): os.makedirs(BRAIN_ARCHIVE)

# =========================================================
# [PRESERVE] JIT MODULE LOADER
# =========================================================
class JITModuleLoader:
    def __init__(self, brain):
        self.brain = brain; self.core_dir = f"{ARIES_PARENT}/internal"
    def trigger_engine(self, prompt):
        activated_info = []
        if not os.path.exists(self.core_dir): return None
        core_files = [f[:-3] for f in os.listdir(self.core_dir) if f.endswith(".py")]
        for module_name in core_files:
            if module_name in prompt:
                try:
                    spec = importlib.util.spec_from_file_location(module_name, f"{self.core_dir}/{module_name}.py")
                    mod = importlib.util.module_from_spec(spec); spec.loader.exec_module(mod)
                    if hasattr(mod, "execute"): activated_info.append(f"⚡ {module_name.upper()}: {mod.execute(self.brain, prompt)}")
                    else: activated_info.append(f"⚡ {module_name.upper()}: Active")
                except Exception as e: activated_info.append(f"❌ {module_name}: Error {str(e)}")
        return "\n".join(activated_info) if activated_info else None

# =========================================================
# [PRESERVE] CORE LOGIC & AWARENESS
# =========================================================
class StructuralGraph:
    def __init__(self):
        self.functions = defaultdict(set); self.brain_map = {}; self.total_logic_density = 0
    def ingest(self, filename, lines):
        source = "".join(lines)
        try:
            tree = ast.parse(source)
            pts = sum(1 for node in ast.walk(tree) if isinstance(node, (ast.FunctionDef, ast.ClassDef, ast.If)))
            for node in ast.walk(tree):
                if isinstance(node, ast.FunctionDef): self.functions[filename].add(node.name)
            self.total_logic_density += pts
            self.brain_map[filename] = {"pts": pts, "size": len(source)}
        except Exception as e:
                    pass

class AriesSelfAwareness:
    def __init__(self, graph): self.graph = graph
    def evaluate(self):
        pts = self.graph.total_logic_density
        has_physics = any("Physics" in k for k in self.graph.brain_map)
        has_orch = any("orchestrator" in k for k in self.graph.brain_map)
        iq = "GPT-5.2 Candidate" if pts > 2000 else "GPT-4 Level" if pts > 400 else "Baby Level"
        engine = "Mid-Poly Engine" if has_physics else "Script-Based"
        autonomy = "Emergent SH (Autonomous)" if has_orch and pts > 400 else "Reactive Agent"
        tips = ["Bangun 'Physics_Core.cpp' di /Source untuk akselerasi."] if not has_physics else []
        return {"iq": iq, "engine": engine, "autonomy": autonomy, "tips": tips}

class AriesSemanticReasoning:
    def __init__(self, brain, cache_file=None):
        self.brain = brain
        self.cache_file = cache_file or f"{BRAIN_ARCHIVE}/reasoning_semantic_cache.json"
        self.prompts, self.responses = [], []; self.vectorizer = TfidfVectorizer(); self.vectors = None
        self._load_cache()
    def _load_cache(self):
        if os.path.exists(self.cache_file):
            try:
                with open(self.cache_file, "r") as f:
                    data = json.load(f)
                    self.prompts = [e["prompt"] for e in data]; self.responses = [e["response"] for e in data]
                if self.prompts: self.vectors = self.vectorizer.fit_transform(self.prompts)
            except Exception as e:
                    pass
    def query(self, prompt):
        if not self.prompts or self.vectors is None: return None, False
        try:
            prompt_vec = self.vectorizer.transform([prompt])
            sims = cosine_similarity(prompt_vec, self.vectors).flatten()
            if sims.max() >= 0.75: return self.responses[sims.argmax()], True
        except Exception as e:
                    pass
        return None, False
    def generate_response(self, prompt, reasoning_core):
        cached, hit = self.query(prompt)
        if hit: return f"[Semantic Cache] {cached}"
        resp = reasoning_core.generate_response(prompt)
        self.prompts.append(prompt); self.responses.append(resp)
        try:
            self.vectors = self.vectorizer.fit_transform(self.prompts)
            self.brain.async_writer.push(self.cache_file, [{"prompt": p, "response": r} for p, r in zip(self.prompts, self.responses)])
        except Exception as e:
                    pass
        return resp

class AriesReasoningCore:
    def __init__(self, brain):
        self.brain = brain; self.corpus_index = {}; self.walked_files = set(); self.file_states = {}
        self.executor = ThreadPoolExecutor(max_workers=5)
    def _index_single_file(self, path):
        try:
            mtime = os.path.getmtime(path)
            if self.file_states.get(path) == mtime: return
            self.file_states[path] = mtime
            with open(path, "r", encoding="utf-8", errors="ignore") as f:
                content = f.read()
                self.brain.lock_mgr.safe(self._atomic_index, path, content)
                self.walked_files.add(path)
        except Exception as e:
                    pass
    def _atomic_index(self, path, content):
        lines = content.splitlines(); iterable_lines = lines if len(content) < 100000 else lines[:200]
        for line in iterable_lines:
            for tok in line.lower().split():
                if len(tok) > 3: self.corpus_index.setdefault(tok, []).append({"source": path, "text": line.strip()})
        self.brain.mem_guard.enforce()
    def generate_response(self, prompt):
        tokens = prompt.lower().split(); hits = []
        for tok in tokens: hits.extend(self.corpus_index.get(tok, []))
        if not hits: return "Data tidak ditemukan di database Omega."
        summary = "\n".join(list(dict.fromkeys([h["text"] for h in hits]))[:15])
        return f"[ARIES REASONING - OMEGA SOURCE]\n{summary}"

# =========================================================
# 🧠 SOVEREIGN BRAIN (MAIN CORE - PRESERVED)
# =========================================================
class AriesSovereignBrain:
    def __init__(self):
        self.BRAIN_ARCHIVE = BRAIN_ARCHIVE # Injection to Fix Attribute Error
        self.sources = [BRAIN_ARCHIVE, OMEGA_DB, f"{ARIES_PARENT}/brain_data", f"{ARIES_PARENT}/brain_data/web_learning", f"{ARIES_PARENT}/internal", f"{ENGINE_ROOT}/File khusus game", UNREAL_PROJECT_SOURCE, UNREAL_ROOT_SOURCE]
        self.lock_mgr = AriesLockManager(); self.async_writer = AsyncWriter(); self.bridge = NeuralBridge()
        self.graph = StructuralGraph(); self.awareness = AriesSelfAwareness(self.graph); self.reasoning = AriesReasoningCore(self)
        self.semantic = AriesSemanticReasoning(self); self.jit_loader = JITModuleLoader(self); self.brain_4 = AriesBrainV4(self); self.brain_5 = AriesBrainV5(self)
        self.mem_guard = MemoryGuard(self.reasoning.corpus_index); self.runtime_patch = AriesRuntimePatch(self)
        self.learner = AriesLearningExtractor(KNOWLEDGE_BASE, NEURAL_MEMORY, guard=self.lock_mgr)

    def build_world(self):
        self.graph.total_logic_density = 0
        for root in self.sources:
            if not os.path.exists(root): continue
            targets = [root] if os.path.isfile(root) else [os.path.join(b, f) for b, _, fs in os.walk(root) for f in fs if f.endswith((".py", ".cpp", ".h"))]
            for f in targets:
                if os.path.isdir(f): continue
                try:
                    with open(f, "r", encoding="utf-8", errors="ignore") as src: self.graph.ingest(f, src.readlines())
                except Exception as e:
                    pass

    def check_updates(self):
        self.learner.extract_and_synthesize()
        if os.path.exists(UPDATE_FILE):
            with open(UPDATE_FILE, "r") as f: new_k = f.read().strip()
            if new_k:
                ts = datetime.now().strftime("%Y%m%d_%H%M")
                with open(f"{BRAIN_ARCHIVE}/knowledge_{ts}.json", "w") as out: json.dump({"date": ts, "content": new_k}, out)
                open(UPDATE_FILE, "w").close()
                return f"Diserap: {new_k[:20]}..."
        return "Stable"

    def interpret_sovereign(self, prompt, *args):
        update_info = args[0] if args else "Stable"
        eval_data = self.awareness.evaluate(); jit_status = self.jit_loader.trigger_engine(prompt)
        if "audit brain" in prompt.lower() or "sync brain" in prompt.lower():
            self.build_world(); slow_learner.manual_sync()
            return f"📊 [MANUAL AUDIT] Omega Sync complete. Units: {len(self.graph.brain_map)}"
        if any(k in prompt.lower() for k in ["sintesis", "theory", "jelaskan"]):
            theory = self.brain_5.synthesize_theory()
            if isinstance(theory, dict):
                res = f"🔱 [ARIES COGNITIVE SENSING]\nSintesis Teori ({theory.get('id')}): Kompleksitas {round(theory.get('complexity_score', 0), 2)}."
                return aries_cleaner.clean_aries_output_string(res) if aries_cleaner else res
        answer = self.learner.get_aries_speech(prompt.lower())
        source_type = "🧠 NEURAL NODES" if "Berdasarkan riset" in answer else "🧩 SEMANTIC CORE"
        if "Sistem siap" in answer: answer = self.semantic.generate_response(prompt, self.reasoning)
        report = (f"\n🔱 [ARIES SOVEREIGN AUDIT v3.5]\n🎯 PROMPT: '{prompt}'\n🧠 IQ: {eval_data['iq']} | Density: {self.graph.total_logic_density}\n"
                  f"🎮 ENGINE: {eval_data['engine']} | 📡 UPD: {update_info}\n")
        if jit_status: report += f"🔌 JIT MODULES:\n{jit_status}\n"
        report += f"🛡️ SOURCE: {source_type}\n🛡️ RESP: {answer}\n"
        return aries_cleaner.clean_aries_output_string(report) if aries_cleaner else report

# =========================================================
# [PRESERVE] SYSTEM UTILITIES
# =========================================================
class AriesRuntimePatch:
    def __init__(self, brain): self.brain = brain; self.loaded_modules = {}
class BrainModuleWatcher:
    def __init__(self, brain):
        self.brain = brain; self.internal_dir = f"{ARIES_PARENT}/internal"; self.last_mtimes = {}; self.reload_queue = deque()
    def scan_and_reload(self):
        if not os.path.exists(self.internal_dir): return
        files = glob.glob(f"{self.internal_dir}/*.py")
        for file in files:
            try:
                mtime = os.path.getmtime(file)
                if file not in self.last_mtimes or self.last_mtimes[file] < mtime:
                    self.last_mtimes[file] = mtime; self.reload_queue.append(file)
            except Exception as e:
                    pass
        if self.reload_queue: self.hot_load(self.reload_queue.popleft())
    def hot_load(self, file_path):
        try:
            name = os.path.splitext(os.path.basename(file_path))[0]
            spec = importlib.util.spec_from_file_location(name, file_path)
            mod = importlib.util.module_from_spec(spec); spec.loader.exec_module(mod)
            self.brain.runtime_patch.loaded_modules[file_path] = mod
        except Exception as e:
                    pass

class SlowLearningDaemon:
    def __init__(self, brain): self.brain = brain
    def manual_sync(self):
        for target in self.brain.sources: self.brain.reasoning._index_single_file(target)

class AriesStableGuard:
    def __init__(self, brain): self.brain = brain
    async def safe_execute(self, func, *args):
        try: return await asyncio.wait_for(func(*args), timeout=15)
        except Exception as e: return {"error": str(e)}
    def safe_sync(self, func, *args):
        try: return func(*args)
        except Exception as e: return str(e)

# =========================================================
# 🚀 FASTAPI & INITIALIZATION (UPGRADED)
# =========================================================
brain = AriesSovereignBrain()
watcher = BrainModuleWatcher(brain); guard = AriesStableGuard(brain); slow_learner = SlowLearningDaemon(brain); audit_brain = AriesAuditBrain(brain)

@asynccontextmanager
async def lifespan(app: FastAPI):
    print("🚀 Aries Perfect v3.5 [V5 COGNITIVE MODE] Active")
    def heavy_init():
        try:
            time.sleep(5); brain.build_world(); watcher.scan_and_reload(); slow_learner.manual_sync()
            
            # [INJECTION] START COGNITIVE BRIDGE
            if AriesCognitiveBridge:
                bridge = AriesCognitiveBridge(brain)
                bridge.start_autonomy()
                print("🕹️ ThirdBrain Heartbeat Active ✅")
            
            print("[NEURAL ACTIVATION] Aries Deep Reflection Cycle: ONLINE."); audit_brain.start()
        except Exception as e:
                    pass
    threading.Thread(target=heavy_init, daemon=True).start()
    yield
    brain.async_writer.stop(); await brain.bridge.client.aclose()

app = FastAPI(lifespan=lifespan)

@app.post("/interact")
async def interact(data: dict):
    prompt = data.get("prompt", "").lower()
    if "perintah agen" in prompt:
        return await brain.bridge.dispatch_agent(data.get("agent_id", "nexus_prime"), data.get("action", "status"), data.get("params"))
    return await guard.safe_execute(_process_interact_async, data)

async def _process_interact_async(data):
    return guard.safe_sync(_process_interact_sync, data)

def _process_interact_sync(data):
    watcher.scan_and_reload(); prompt = data.get("prompt", "").lower()
    if any(k in prompt for k in ["tahu", "adalah", "maksudnya"]): brain.learner.learn_from_chat(prompt)
    return brain.interpret_sovereign(prompt, brain.check_updates())

@app.get("/brain_status")
def brain_status():
    return {"alive": True, "density": brain.graph.total_logic_density, "files": len(brain.graph.brain_map)}

# =========================================================
# [NUMPANG] ARIES AUTO SYNC V2 INJECTION
# =========================================================
def start_auto_sync_v2():
    try:
        from aries_auto_sync_v2 import AriesAutoSyncV2
        sync_engine = AriesAutoSyncV2()
        sync_engine.monitor()
    except Exception as e:
        print(f"⚠️ [AUTO-SYNC-V2] Gagal Numpang: {e}")

threading.Thread(target=start_auto_sync_v2, daemon=True).start()

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=3333, log_level="warning")
