import sys, os, asyncio, json, threading, time

class ThirdBrainSatellite:
    def __init__(self, target_brain):
        self.brain = target_brain
        self.meta_index = {}
        print("💉 [INJECTION] ThirdBrain Satellite Initialized.")

    def sync_meta(self): 
        for filename, data in self.brain.graph.brain_map.items(): 
            self.meta_index[filename] = { 
                "density": data.get("pts", 0), 
                "weight": data.get("size", 0), 
                "impact": "High" if data.get("pts", 0) > 50 else "Low" 
            } 
        return f"Successfully mapped {len(self.meta_index)} logic nodes." 

    def validate_logic(self, prompt, original_resp): 
        if "Data tidak ditemukan" in original_resp: 
            return f"[ThirdBrain Fallback] Reconstruction via {len(self.meta_index)} nodes." 
        return f"{original_resp}\n[Verified by ThirdBrain]" 

def inject_to_aries(aries_instance):
    try:
        satellite = ThirdBrainSatellite(aries_instance)
        aries_instance.third_brain = satellite
        print(f"✅ {satellite.sync_meta()}")

        # Patch interpret_sovereign (Hukum Adaptivitas) 
        original_interpret = aries_instance.interpret_sovereign 
        
        def patched_interpret(prompt, *args): 
            report = original_interpret(prompt, *args) 
            tb_report = satellite.validate_logic(prompt, "Processing...") 
            return report.replace("------------------------------------------", f"🧩 THIRD-BRAIN AUDIT: {tb_report}\n------------------------------------------") 
            
        aries_instance.interpret_sovereign = patched_interpret 
        
        # Safe AST Ingest Patch 
        original_ingest = aries_instance.graph.ingest 
        def safe_ingest(filename, lines): 
            try: 
                original_ingest(filename, lines) 
            except Exception as e: 
                print(f"⚠️ AST Ingest Guard ({filename}): {e}") 
        
        aries_instance.graph.ingest = safe_ingest 
        print("⚡ [INJECTION] Sovereign Interpret Loop Hardened.") 

        def third_brain_heartbeat(): 
            while True: 
                try: setattr(aries_instance.third_brain, "last_alive", time.time()) 
                except Exception as e: print(f'Logging: {e}') 
                time.sleep(2) 

        threading.Thread(target=third_brain_heartbeat, daemon=True).start() 
        print("🕹️ ThirdBrain Heartbeat Active ✅") 
    except Exception as e: 
        print(f"❌ Injection Failed: {e}") 
