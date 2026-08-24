import threading
import time
import random

class AriesAuditBrain(threading.Thread):
    def __init__(self, engine_ref, **kwargs):
        # Bersihkan spam guard. Panggil super init dengan benar.
        super().__init__()
        self.engine = engine_ref
        self.learner = engine_ref.learner
        self.guard = kwargs.get('guard', getattr(engine_ref, 'guard', None))
        self.interval = 1920  # 32 Menit (Avoid collision with V4/V5)
        self.daemon = True
        self._stop_event = threading.Event()

    def stop(self):
        """Graceful shutdown untuk Enterprise Stability."""
        self._stop_event.set()

    def run(self):
        print("[NEURAL ACTIVATION] Aries Deep Reflection Cycle: ONLINE.")
        while not self._stop_event.is_set():
            try:
                # Cek guard sebelum eksekusi cycle dengan pengecekan callable agar aman
                if self.guard and hasattr(self.guard, 'allow') and callable(self.guard.allow):
                    if not self.guard.allow("audit_reflection"):
                        time.sleep(60)
                        continue

                # 1. Ekstrak Ilmu (Incremental)
                # 1. Ekstrak Ilmu (Incremental) & Capture count
                count = self.learner.extract_and_synthesize()

                # 3. Memory Compaction
                if hasattr(self.engine, 'mem_guard') and hasattr(self.engine.mem_guard, 'enforce'):
                    self.engine.mem_guard.enforce()

                print(f"[ARIES REFLECTION] Cycle Success: {count} nodes processed.")
            except Exception as e:
                print(f"[AUDIT ERROR] {e}")

            # Jitter sleep untuk efisiensi CPU
            sleep_time = self.interval + random.randint(-60, 60)
            if self._stop_event.wait(sleep_time):
                break
