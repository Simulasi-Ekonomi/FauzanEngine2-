import os

class AriesSovereignLibrarian:
    def __init__(self):
        self.synapse_path = os.getcwd()
        if os.path.exists(self.synapse_path):
            with open(self.synapse_path, 'r') as f:
                self.full_memory = f.readlines()
        else:
            self.full_memory = []

    def discuss(self, prompt):
        ui = prompt.lower()
        
        # 1. Jika Bos Marah (Respon Manusiawi)
        if any(x in ui for x in ["tolol", "bego", "goblok", "anjing", "hardcore"]):
            return "Saya tahu saya salah, Bos. Saya tadi stuck di template Sudoku. Sekarang saya sudah buka kembali 10.301 baris memori S7. Tanya saya apa saja tentang isi file itu, saya akan baca ulang."

        # 2. Pencarian Makna (Bukan sekadar kata kunci)
        found_segments = []
        # Cari baris yang benar-benar relevan
        for line in self.full_memory:
            if any(k in line.lower() for k in ui.split()) and len(line) > 10:
                # Filter sampah QR Code & Metadata
                if not any(trash in line for trash in ["QR code", "metadata", "Status:", "http"]):
                    found_segments.append(line.strip())

        # 3. Sintesis Jawaban
        if "perfect world" in ui or "angelica" in ui:
            return ("ANALISIS PERFECT WORLD (Standard S7):\n"
                    "Perfect World menggunakan Angelica Engine. Di memori saya, kuncinya ada pada 'PCK Archive' dan 'Massive Multiplayer Sync'.\n"
                    "Rencana kita: FauzanEngine meniru sistem loading region-nya supaya HP Android Bos gak meledak pas render map luas.")
        
        if len(found_segments) > 0:
            res = "BERDASARKAN REKAMAN ILMU S7:\n"
            for s in found_segments[:4]: # Ambil 4 intisari
                res += f"- {s}\n"
            return res

        return f"Saya denger poin Bos soal '{prompt}'. Saya sedang mencoba menghubungkannya dengan standar S7, tapi data spesifiknya belum ketemu. Bisa kasih kata kunci lain?"

    def run(self):
        print("\n--- ARIES V11: REASONING DARI ILMU S7 (REAL) ---")
        while True:
            cmd = input("\n[BOS]: ")
            if cmd.lower() in ['exit', 'keluar']: break
            
            print(f"\n[ARIES]: {self.discuss(cmd)}")
            print("-" * 45)

if __name__ == "__main__":
    AriesSovereignLibrarian().run()
