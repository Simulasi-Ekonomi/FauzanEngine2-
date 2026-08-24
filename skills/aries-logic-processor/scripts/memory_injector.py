import os

source_mem = "/sdcard/Buku saya/FauzanEngine/aries_core_synapse.txt"
ref_dir = "/sdcard/Buku saya/FauzanEngine/skills/aries-logic-processor/references"

def inject():
    if not os.path.exists(source_mem):
        print("❌ File memori 10k tidak ditemukan!")
        return

    with open(source_mem, 'r') as f:
        lines = f.readlines()

    # Kategori filter agar tidak hardcore
    categories = {
        "td_logic": ["pathfinding", "grid", "tower", "creep", "wave"],
        "engine_tech": ["aabb", "collision", "vertex", "memory guard", "pck"],
        "art_style": ["sprite", "atlas", "animation", "frame"]
    }

    storage = {k: [] for k in categories}

    for line in lines:
        # Filter: Jangan ambil baris yang isinya QR Code atau log sampah
        if any(trash in line.lower() for trash in ["qr code", "status: locked", "http"]):
            continue
            
        for cat, keywords in categories.items():
            if any(key in line.lower() for key in keywords):
                storage[cat].append(line.strip())

    # Tulis ke file referensi (Hanya ambil 50 intisari terbaik per kategori agar tidak bloat)
    for cat, data in storage.items():
        with open(f"{ref_dir}/{cat}.md", "w") as f:
            f.write(f"# S7 Reference: {cat.replace('_', ' ').upper()}\n\n")
            for item in data[:50]: 
                f.write(f"- {item}\n")
    
    print("✅ Injeksi Memori Selesai. Data 10k telah diperas menjadi modul cerdas.")

if __name__ == "__main__":
    inject()
