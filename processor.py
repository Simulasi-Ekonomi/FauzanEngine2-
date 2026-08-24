import os
import sys
import importlib.util

class NeoEngineAries:
    def __init__(self):
        self.identity = "Aries Sovereign Brain v3.5"
        self.architecture = "3-Tier (React Frontend, FastAPI Backend, Vulkan C++ Runtime)"
        self.mem_path = "/sdcard/Buku saya/FauzanEngine/aries_core_synapse.txt"
        self.vault_path = "/sdcard/Buku saya/FauzanEngine/skills/aries-logic-processor/logic_vault"

    def import_skill(self, skill_name):
        """Fungsi untuk menambah/import skill dari folder logic_vault"""
        try:
            module_path = os.path.join(self.vault_path, f"{skill_name}.py")
            if os.path.exists(module_path):
                spec = importlib.util.spec_from_file_location(skill_name, module_path)
                module = importlib.util.module_from_spec(spec)
                spec.loader.exec_module(module)
                return module
        except Exception as e:
            print(f"⚠️ Gagal import skill {skill_name}: {e}")
        return None

    def synthesize(self, prompt):
        ui = prompt.lower()
        res = [f"### 🤖 {self.identity}", f"**Mode**: {self.architecture}\n"]

        # Logika Inti
        if any(x in ui for x in ["game", "world"]):
            res.append("#### [STRATEGI]\n- Director: aries_director.py\n- World: Ruflo (FastNoiseLite)")

        # Penambahan Skill Dinamis
        elif "belajar" in ui or "study" in ui:
            skill = self.import_skill("study_core")
            if skill:
                res.append(skill.StudySkill().execute(self.mem_path))
            else:
                res.append("❌ Skill belajar belum di-import atau file tidak ada.")

        elif "audit" in ui or "bug" in ui:
            skill = self.import_skill("audit_system")
            if skill:
                res.append(skill.AuditSkill().execute(self.mem_path))
            else:
                res.append("❌ Skill audit belum di-import.")

        # --- MATA BARU: DEEP SCANNER ---
        elif any(x in ui for x in ["scan", "pindai", "seluruh"]):
            skill = self.import_skill("deep_scanner")
            if skill:
                # Menjalankan pemindaian pada root engine
                res.append(skill.DeepScannerSkill().execute(self.mem_path))
            else:
                res.append("❌ Skill scanner belum di-import.")
        # -------------------------------

        elif "terapkan" in ui or "eksekusi" in ui:
            skill = self.import_skill("action_executor")
            if skill:
                res.append(skill.ActionExecutorSkill().execute(self.mem_path))
            else:
                res.append("❌ Skill eksekutor tidak ditemukan.")

        else:
            res.append("Instruksi: 'game', 'belajar', 'audit bug', 'pindai seluruh', atau 'eksekusi'.")

        return "\n".join(res)

if __name__ == "__main__":
    if len(sys.argv) > 1:
        print(NeoEngineAries().synthesize(" ".join(sys.argv[1:])))
