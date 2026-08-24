import os
import time
import hashlib
import json
from datetime import datetime

# Import Core lo
from aries_knowledge_inductor_pro import AriesInductorPro
from aries_brain_final_polisher import AriesBrainFinalPolisher

class AriesAutoSyncV2:
    def __init__(self):
        self.processed_dir = os.getcwd()
        self.master_kb_path = os.getcwd()
        self.sync_log = os.getcwd()
        
        # Inisialisasi ingatan (hash) agar tidak scan ulang
        self.known_hashes = self._load_sync_log()

    def _load_sync_log(self):
        if os.path.exists(self.sync_log):
            try:
                with open(self.sync_log, 'r') as f:
                    return json.load(f).get("processed_files", {})
            except: return {}
        return {}

    def _save_sync_log(self, file_dict):
        with open(self.sync_log, 'w') as f:
            json.dump({
                "last_sync": datetime.now().isoformat(),
                "processed_files": file_dict
            }, f, indent=2)

    def _get_file_hash(self, path):
        hasher = hashlib.md5()
        with open(path, 'rb') as f:
            buf = f.read()
            hasher.update(buf)
        return hasher.hexdigest()

    def check_for_updates(self):
        if not os.path.exists(self.processed_dir):
            return False

        current_files = [f for f in os.listdir(self.processed_dir) if f.endswith('.json')]
        has_new_data = False
        
        new_file_map = self.known_hashes.copy()

        for fname in current_files:
            fpath = os.path.join(self.processed_dir, fname)
            fhash = self._get_file_hash(fpath)
            
            # Jika file belum pernah dilihat atau isinya berubah (hash beda)
            if fname not in self.known_hashes or self.known_hashes[fname] != fhash:
                print(f"[{datetime.now().strftime('%H:%M:%S')}] 🆕 Perubahan terdeteksi: {fname}")
                new_file_map[fname] = fhash
                has_new_data = True

        if has_new_data:
            self.known_hashes = new_file_map
            return True
        return False

    def run_sync_pipeline(self):
        print(f"[{datetime.now().strftime('%H:%M:%S')}] 🚀 Memulai Pipeline Auto-Sync...")
        
        # 1. Jalankan Inductor Pro (Ekstraksi ke Master KB)
        try:
            inductor = AriesInductorPro(self.processed_dir)
            inductor.start_induction()
        except Exception as e:
            print(f"❌ Inductor Error: {e}")
            return

        # 2. Jalankan Polisher (Pecah Master KB ke Segments)
        try:
            polisher = AriesBrainFinalPolisher(self.master_kb_path)
            polisher.run_polishing()
            print(f"[{datetime.now().strftime('%H:%M:%S')}] 💎 Segmen Otak Berhasil Diperbarui.")
        except Exception as e:
            print(f"❌ Polisher Error: {e}")
            return

        # Simpan state agar tidak diulang
        self._save_sync_log(self.known_hashes)
        print(f"[{datetime.now().strftime('%H:%M:%S')}] ✅ Sinkronisasi Selesai. Menunggu data baru...")

    def monitor(self):
        print(f"📡 Aries Auto-Sync V2 Monitoring: {self.processed_dir}")
        print("CTRL+C untuk berhenti.\n")
        
        while True:
            if self.check_for_updates():
                self.run_sync_pipeline()
            
            # Polling tiap 5 detik biar nggak makan baterai/CPU
            time.sleep(5)

if __name__ == "__main__":
    sync_engine = AriesAutoSyncV2()
    try:
        sync_engine.monitor()
    except KeyboardInterrupt:
        print("\n[!] Auto-Sync Dihentikan.")
