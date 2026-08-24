import os
import time
import threading

class AriesBrainV4:
    def __init__(self, brain_owner):
        self.brain = brain_owner
        self.log_path = "/sdcard/Buku saya/FauzanEngine/backend/aries/brain_v4_history.log"
        self.targets = [
            "/sdcard/Buku saya/FauzanEngine/",
            "/sdcard/Buku saya/File khusus game/"
        ]
        self.supported_extensions = ['.json', '.py', '.txt', '.cpp', '.h']
        self._load_history()
        threading.Thread(target=self._initial_delay_start, daemon=True).start()

    def _initial_delay_start(self):
        time.sleep(600)
        while True:
            self.deep_learning_scan()
            time.sleep(600)

    def _load_history(self):
        if os.path.exists(self.log_path):
            with open(self.log_path, 'r') as f:
                self.history = set(line.strip() for line in f)
        else:
            self.history = set()

    def deep_learning_scan(self):
        print("🔍 V4: Starting Scheduled Scan...")
        total = 0
        for base in self.targets:
            if not os.path.exists(base): continue
            for root, _, files in os.walk(base):
                for f in files:
                    path = os.path.join(root, f)
                    if path in self.history: continue
                    if not any(path.endswith(e) for e in self.supported_extensions): continue
                    try:
                        with open(path, 'r', encoding='utf-8', errors='ignore') as file:
                            content = file.read()
                            if content:
                                self._integrate(path, content)
                                os.makedirs(os.path.dirname(self.log_path), exist_ok=True)
                                with open(self.log_path, 'a') as log:
                                    log.write(path + "\n")
                                self.history.add(path)
                                total += 1
                                if total % 10 == 0: time.sleep(0.2)
                    except: continue
        print(f"✅ V4: Scan Complete. {total} new files learned.")

    def _integrate(self, path, content):
        tokens = content.lower().split()
        snippet = " ".join(tokens[:300])
        keyword = tokens[0] if tokens and len(tokens[0]) > 3 else "theory"
        idx = self.brain.reasoning.corpus_index
        if keyword not in idx: idx[keyword] = []
        idx[keyword].append({"source": path, "text": f"[INSIGHT] {snippet}"})
