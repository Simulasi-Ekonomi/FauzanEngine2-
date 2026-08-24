import os
import re

class StudySkill:
    def execute(self, mem_path):
        if not os.path.exists(mem_path): return "❌ Memori S7 tidak ditemukan."
        
        with open(mem_path, 'r') as f:
            content = f.read()

        # Ekstraksi Pengetahuan
        lessons = {
            "C++": len(re.findall(r"(std::|void|int|class|public:|private:)", content)),
            "Python": len(re.findall(r"(def |import |self\.|elif|if __name__)", content)),
            "Game Tech": len(re.findall(r"(vulkan|rhi|ecs|aabb|mesh|shader|texture)", content.lower()))
        }
        
        # Simpan hasil belajar ke file index kecil untuk referensi skill lain
        index_path = "/sdcard/Buku saya/FauzanEngine/skills/aries-logic-processor/brain_index.txt"
        with open(index_path, 'w') as f:
            for k, v in lessons.items():
                f.write(f"{k}:{v}\n")

        res = ["#### 📚 ARIES LEARNING SESSION (S7 PROGRESS)"]
        res.append(f"- Memahami {lessons['C++']} struktur C++ (Pointer, Class, Memory).")
        res.append(f"- Menguasai {lessons['Python']} pola Python (Scripting, Automation).")
        res.append(f"- Menyerap {lessons['Game Tech']} konsep Game Engine (Rendering, Physics).")
        res.append("\n**Status**: Pengetahuan telah diindeks. Aries sekarang lebih peka terhadap konteks teknis.")
        
        return "\n".join(res)
