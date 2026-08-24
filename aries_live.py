import os
import json
import requests
import time

# --- ARIES LIVE SOVEREIGN v2.7 (BYPASS 403 PROTOCOL) ---
# Environment-based API injection untuk Master Dikri Fauzan
API_KEY = "" # Sistem akan menginjeksi key saat runtime
MODEL = "gemini-2.5-flash-preview-09-2025"
URL = f"https://generativelanguage.googleapis.com/v1beta/models/{MODEL}:generateContent?key={API_KEY}"

def aries_print(msg, mode="info"):
    colors = {"info": "\033[34m", "success": "\033[32m", "error": "\033[31m", "search": "\033[36m"}
    print(f"{colors.get(mode, '\033[0m')}[Aries] {msg}\033[0m")

def live_learn(query):
    aries_print(f"Mengaktifkan protokol pencarian real-time untuk: {query}...", "search")
    
    # Payload dengan Google Search Grounding aktif
    payload = {
        "contents": [{
            "parts": [{
                "text": f"Gunakan pencarian Google untuk mempelajari secara detail tentang '{query}'. Berikan laporan teknis standar industri (C++, Unreal Engine, atau Game Dev) dan tampilkan daftar URL referensi yang kamu gunakan."
            }]
        }],
        "tools": [{ "google_search": {} }]
    }

    try:
        # Request dengan penanganan timeout industri
        response = requests.post(URL, json=payload, timeout=60)
        
        if response.status_code != 200:
            aries_print(f"Error {response.status_code}: Pastikan dijalankan di dalam lingkungan Canvas/Aries yang terhubung.", "error")
            if response.status_code == 403:
                print("\033[33m[Saran Master]: Jalankan skrip ini melalui UI Preview Aries agar API Key terinjeksi otomatis.\033[0m")
            return

        res_data = response.json()
        candidate = res_data.get('candidates', [{}])[0]
        content = candidate.get('content', {}).get('parts', [{}])[0].get('text', 'Informasi tidak ditemukan.')
        metadata = candidate.get('groundingMetadata', {})
        sources = metadata.get('groundingAttributions', [])

        print("\n" + "═" * 70)
        print("\033[1;32m[ARIES LIVE KNOWLEDGE REPORT]\033[0m")
        print(content)
        print("═" * 70)

        if sources:
            print("\033[1;33m[SUMBER DATA INTERNET]:\033[0m")
            for attr in sources:
                title = attr.get('web', {}).get('title', 'Dokumentasi Terkait')
                uri = attr.get('web', {}).get('uri', '#')
                print(f"🔗 {title}: {uri}")
        print("═" * 70 + "\n")

    except Exception as e:
        aries_print(f"Gagal menghubungkan ke Otak Pusat: {str(e)}", "error")

def main():
    os.system('clear')
    print("\033[1;34m" + "╔" + "═"*68 + "╗")
    print("║                  ARIES LIVE SOVEREIGN v2.7 (FIXED)                 ║")
    print("║            STANDAR INDUSTRI - NEO ENGINE - FAUZANENGINE            ║")
    print("╚" + "═"*68 + "╝\033[0m")
    
    while True:
        try:
            prompt = input("\033[1;32m[Master @ FauzanEngine] > \033[0m")
            if not prompt: continue
            if prompt.lower() in ['exit', 'quit']: break
            
            live_learn(prompt)
        except KeyboardInterrupt:
            break

if __name__ == "__main__":
    main()
