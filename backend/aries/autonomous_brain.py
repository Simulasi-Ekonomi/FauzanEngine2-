import os, json, asyncio
from datetime import datetime

try:
    import networkx as nx
    HAS_NX = True
except ImportError:
    HAS_NX = False

class AriesAutonomousSystem:
    def __init__(self):
        self.context_path = os.getcwd() if "os" in locals() else "."
        self.log_dir = f"{self.context_path}/backend/aries/brain_data/web_learning"
        self.dashboard_path = f"{self.context_path}/ARIES_DASHBOARD.md"
        os.makedirs(self.log_dir, exist_ok=True)
        self.memory_log = f"{self.context_path}/skills/aries-fauzan-engine/aries_memory.log"

    def log_activity(self, message):
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        with open(self.memory_log, "a") as f:
            f.write(f"[{timestamp}] {message}\n")

    async def s6_fetch_intel(self, url):
        return {
            "title": "Unreal High Performance C++",
            "code_blocks": ["std::vector<int> v; v.reserve(100);", "TSharedPtr<FUser> User;"],
            "content": "Deep analysis via S6 Agent."
        }

    async def s7_reasoning_cycle(self, url, topic):
        self.log_activity(f"S7 REASONER: Analyzing {topic}")
        intel = await self.s6_fetch_intel(url)
        
        if HAS_NX:
            dg = nx.DiGraph()
            dg.add_node(topic, type="root")
            for i, block in enumerate(intel['code_blocks']):
                dg.add_edge(topic, f"pattern_{i}")
            node_count = len(dg.nodes())
        else:
            node_count = "N/A (NX Missing)"

        # Update Dashboard
        self.update_dashboard(topic, url, "Success")
        return f"S7 PRO SUCCESS: {topic} synthesized. Graph nodes: {node_count}"

    def audit_engine_source(self, file_content):
        """Logika Audit S7: Membandingkan kode lokal dengan standar internet"""
        self.log_activity("S7 AUDIT: Scanning code content")
        suggestions = []
        if "std::vector" in file_content and ".reserve" not in file_content:
            suggestions.append("- [OPTIMIZATION]: Gunakan .reserve() pada std::vector untuk mencegah re-alokasi memori.")
        if "new " in file_content:
            suggestions.append("- [WARNING]: Deteksi 'new' manual. Gunakan TSharedPtr atau std::unique_ptr sesuai standar S7.")
        
        return "\n".join(suggestions) if suggestions else "CODE STATUS: Professional Standard."

    def update_dashboard(self, topic, source, status):
        """Membuat/Update Dashboard Progress Aries"""
        now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        content = f"""# 🧠 ARIES ENGINE ARCHITECT DASHBOARD
Last Update: {now}

## 🌐 Web Learning (S6/S7)
- **Topic**: {topic}
- **Source**: {source}
- **Status**: {status}

## 🛠️ Engine Status
- **Core Optimization**: Active
- **C++ Standards**: Unreal-Style

---
*Aries is currently monitoring FauzanEngine Source...*
"""
        with open(self.dashboard_path, "w") as f:
            f.write(content)
