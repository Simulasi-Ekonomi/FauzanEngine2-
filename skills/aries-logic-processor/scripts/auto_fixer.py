import os

class NeoFixer:
    def __init__(self):
        self.mem_path = "/sdcard/Buku saya/FauzanEngine/aries_core_synapse.txt"

    def suggest_fixes(self):
        with open(self.mem_path, 'r') as f:
            lines = f.readlines()
        
        fixes = []
        for line in lines:
            if "new " in line and "unique_ptr" not in line and ".cpp" in line:
                # Simulasi konversi ke Smart Pointer
                original = line.strip()
                fixed = original.replace("new ", "std::make_unique<") + ">()"
                fixes.append(f"ORIGINAL: {original}\n   FIXED: {fixed}")
        
        return fixes

if __name__ == "__main__":
    fixer = NeoFixer()
    results = fixer.suggest_fixes()
    print("### 🛠️ ARIES AUTO-FIX RECOMMENDATIONS")
    for r in results[:5]: # Tampilkan 5 sampel teratas
        print(f"\n{r}")
