import os
import sys

class AuditSkill:
    def execute(self, mem_path):
        # 1. Aries 'Mengingat' apa yang dipelajari
        index_path = "/sdcard/Buku saya/FauzanEngine/skills/aries-logic-processor/brain_index.txt"
        knowledge = {}
        if os.path.exists(index_path):
            with open(index_path, 'r') as f:
                for line in f:
                    k, v = line.strip().split(':')
                    knowledge[k] = int(v)

        # 2. Menerapkan ilmu: Jika C++ terdeteksi banyak, audit harus lebih ketat
        with open(mem_path, 'r') as f:
            lines = f.readlines()

        # Gunakan 'ilmu' C++ untuk mencari leak
        leaks = [l for l in lines if "new " in l and "unique" not in l and any(x in l for x in ["class", "void", "int"])]
        
        res = ["#### 🛡️ SECURITY REPORT (Applied Knowledge Mode)"]
        res.append(f"Aries menggunakan pemahaman dari {knowledge.get('C++', 0)} struktur C++.")
        res.append(f"- Temuan: {len(leaks)} kebocoran memori yang tidak sesuai standar S7.")
        
        if leaks:
            res.append("\n**Saran Berdasarkan Ilmu Python/Game Tech:**")
            res.append("- Gunakan pola Factory Pattern untuk alokasi objek agar memory footprint rendah.")
            
        return "\n".join(res)
