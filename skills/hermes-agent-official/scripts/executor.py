import subprocess
import os

def ask_hermes_sovereign(query, yolo=False):
    # -q untuk query, -Q untuk hasil bersih (programmatic)
    # --worktree agar dia paham konteks repo FauzanEngine
    cmd = ["hermes", "chat", "-Q", "-q", query, "--worktree"]
    
    if yolo:
        cmd.append("--yolo")
        
    try:
        # Jalankan di folder FauzanEngine agar --worktree aktif
        result = subprocess.run(
            cmd, 
            cwd="/sdcard/Buku saya/FauzanEngine", 
            capture_output=True, 
            text=True
        )
        return result.stdout.strip()
    except Exception as e:
        return f"Sovereign Bridge Error: {str(e)}"

if __name__ == "__main__":
    # Test: Tanya Hermes tentang status engine kita
    print("--- Aries calling Hermes Opus ---")
    response = ask_hermes_sovereign("Berikan ringkasan struktur folder FauzanEngine dan satu saran optimasi C++.")
    print(response)
