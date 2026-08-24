import time
import threading
import gc
from collections import defaultdict, Counter

class AriesBrainV5:
    def __init__(self, brain):
        self.brain = brain
        self.knowledge_graph = defaultdict(list)
        self.pattern_memory = Counter()
        self.processed_sources = set() # Log internal V5
        threading.Thread(target=self._initial_delay_start, daemon=True).start()

    def _initial_delay_start(self):
        time.sleep(907) # Prime shift preserved
        while True:
            self.absorb_reasoning()
            time.sleep(907)

    def absorb_reasoning(self):
        # REMOVED: if getattr(self.brain.reasoning, "is_locked", False): return
        print("🧠 V5: Starting Incremental Synthesis...")
        corpus = getattr(self.brain.reasoning, "corpus_index", {})
        count = 0
        for token in list(corpus.keys()):
            entries = corpus.get(token, [])
            for item in entries:
                source = item.get("source")
                if source in self.processed_sources:
                    continue
                text = item.get("text", "")
                self.knowledge_graph[token].append(text)
                self.pattern_memory[token] += 1
                self.processed_sources.add(source)
                count += 1
                if count % 20 == 0:
                    time.sleep(0.3)
                    gc.collect() 
        print(f"✅ V5: Incremental Synthesis Complete. Processed {count} new insights.")
        gc.collect()

    def synthesize_theory(self):
        self.absorb_reasoning()
        return {
            "id": "v5_synthesis_cycle",
            "complexity_score": len(self.pattern_memory) * 0.1
        }
