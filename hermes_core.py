import os
import json
import requests
import subprocess
import sys

# =================================================================
# Name        : Hermes Active Core v7.0
# Role        : Active Agent (Chat, Scan, Repair)
# Master      : Dikri Fauzan
# =================================================================

UI_URL = "http://localhost:8080" 

def get_intelligence(prompt):
    print("\033[1;34m[Hermes] Menghubungi UI Cloud untuk riset ilmu...\033[0m")
    try:
        # Mengirim permintaan ke Knowledge Bridge UI
        res = requests.post(f"{UI_URL}/api/knowledge", json={"prompt": prompt}, timeout=30)
        return res.json().get("knowledge", "Gagal mendapatkan data.")
    except Exception as e:
        return f"UI Offline atau Timeout. Pastikan Preview UI aktif di browser. Error: {e}"

def scan_engine():
    print("\033[1;33m[Hermes] Memindai FauzanEngine secara otonom...\033[0m")
    weak_points = []
    # Fokus pada folder Source
    target_dir = "./Source" if os.path.exists("./Source") else "."
    for root, _, files in os.walk(target_dir):
        for f in files:
            if f.endswith((".cpp", ".h")):
                path = os.path.join(root, f)
                with open(path, 'r') as file:
                    lines = file.readlines()
                    for i, line in enumerate(lines):
                        if "strcpy" in line or "malloc" in line or "NULL" in line:
                            weak_points.append({"file": path, "line": i+1, "content": line.strip()})
    return weak_points

def main_loop():
    os.system('clear')
    print("\033[1;36m╔" + "═"*55 + "╗")
    print("║          HERMES ACTIVE CORE v7.0 - SOVEREIGN          ║")
    print("║ Master: Dikri Fauzan | Status: Active Agent           ║")
    print("╚" + "═"*55 + "╝\033[0m")
    
    while True:
        cmd = input("\033[1;32m[Hermes @ FauzanEngine] $ \033[0m").strip()
        
        if not cmd: continue
        
        if cmd.lower() == "scan":
            results = scan_engine()
            print(f"\n\033[1;31m[!] Ditemukan {len(results)} kerentanan kode.\033[0m")
            for r in results:
                print(f" -> {r['file']} (Line {r['line']}): {r['content']}")
            print("\033[1;34mGunakan perintah 'perbaiki [path]' untuk memanggil UI Intelligence.\033[0m")

        elif cmd.lower().startswith("perbaiki "):
            path = cmd.split(" ", 1)[1]
            if os.path.exists(path):
                with open(path, 'r') as f:
                    old_code = f.read()
                
                print(f"[Hermes] Mengirim kode {path} ke UI untuk di-upgrade...")
                new_code = get_intelligence(f"Upgrade kode C++ ini ke standar AAA Engine, perbaiki path jika salah, jangan ubah logika utama: \n\n{old_code}")
                
                with open(path, 'w') as f:
                    f.write(new_code)
                print(f"\033[1;32m[✔] {path} telah diperbaiki dengan ilmu terbaru.\033[0m")
            else:
                print("[!] File tidak ditemukan.")

        elif cmd.lower() == "help":
            print("\nPerintah:")
            print(" - scan             : Deteksi kode lemah")
            print(" - perbaiki [path]  : Upgrade kode via UI Intelligence")
            print(" - chat [pesan]     : Diskusi teori dengan Hermes")
            print(" - exit             : Matikan agen\n")

        elif cmd.lower() == "exit":
            print("Hermes pamit, Master.")
            break
            
        else:
            # Mode Chatting Aktif Hermes
            print(f"\033[1;37m[Hermes]: Saya mengerti Master. Instruksi '{cmd}' akan saya simpan di memori lokal. Apakah kita perlu riset ke UI?\033[0m")

if __name__ == "__main__":
    # Pastikan library requests ada
    try:
        import requests
    except ImportError:
        subprocess.check_call([sys.executable, "-m", "pip", "install", "requests"])
    
    main_loop()
