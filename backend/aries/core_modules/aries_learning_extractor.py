import json
import os
from datetime import datetime
from global_generator_guard import GlobalGeneratorGuard

class AriesLearningExtractor:
    # Ditambahkan "guard" ke slots supaya tidak error saat injection

    def __init__(self, 
                 kb_path=os.getcwd(), 
                 neural_path=os.getcwd(), 
                 **kwargs):
        self.kb_path = kb_path
        self.neural_path = neural_path
        self.guard = kwargs.get("guard", None) # Bisa terima guard langsung saat init
        self.label_map = {
            "math": ["rumus", "hitung", "angka", "logic"],
            "unreal": ["cpp", "source", "actor", "engine", "unreal"],
            "physics": ["gravitasi", "collision", "massa", "force"]
        }

    def _load_json(self, path):
        # Bersihkan spam: Cukup satu cek guard
        if hasattr(self, "guard") and self.guard and not self.guard.allow("_load_json"): 
            return {}

        if os.path.exists(path):
            with open(path, 'r') as f:
                try: 
                    return json.load(f)
                except: 
                    return {}
        return {}

    def extract_and_synthesize(self):
        # Audit Fix: Incremental Indexing (O(N))
        kb_data = self._load_json(self.kb_path)
        neural_mem = self._load_json(self.neural_path)

        if "neural_nodes" not in neural_mem: 
            neural_mem["neural_nodes"] = {}
        
        nodes = neural_mem["neural_nodes"]
        new_count = 0
        
        # Chunking: Maksimal 50 item per siklus untuk mencegah CPU spike
        for key, content in kb_data.items():
            if key in nodes: continue
            if new_count >= 50: break

            content_str = str(content)
            nodes[key] = {
                "content": content_str,
                "labels": [l for l, kws in self.label_map.items() if any(k in content_str.lower() for k in kws)],
                "last_refined": datetime.now().isoformat(),
                "connections": self._build_links(content_str, nodes)
            }
            new_count += 1

        with open(self.neural_path, 'w') as f:
            json.dump(neural_mem, f)
        return new_count

    def _build_links(self, text, existing_nodes):
        if hasattr(self, "guard") and self.guard and not self.guard.allow("_build_links"): 
            return []

        links = []
        words = set(text.lower().split())
        # Audit: Hanya bandingkan dengan 100 node terakhir untuk efisiensi O(100)
        for k, node in list(existing_nodes.items())[-100:]:
            other_words = set(str(node.get("content", "")).lower().split())
            if len(words & other_words) > 4:
                links.append({"to": k, "type": "semantic"})
        return links

    def learn_from_chat(self, prompt):
        # Implementasi sederhana untuk learning dari interaksi
        kb_data = self._load_json(self.kb_path)
        key = f"chat_{int(datetime.now().timestamp())}"
        kb_data[key] = prompt
        with open(self.kb_path, 'w') as f:
            json.dump(kb_data, f)

    def get_aries_speech(self, prompt):
        theories = ["Neural Mesh Efficiency", "Entropi Logika", "Sovereign Invariant"]
        for theory in theories:
            if theory.lower() in prompt.lower():
                return f"Berdasarkan riset neural, penerapan {theory} sedang dioptimasi pada sistem."
        return "Sistem siap. Menunggu instruksi logika lanjutan."
