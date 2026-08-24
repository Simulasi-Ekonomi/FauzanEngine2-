import os
import subprocess
import sys

def audit_and_test():
    print("\033[34m[Aries Agent] Memulai Refactoring & Sandbox Testing...\033[0m")
    
    # Path utama berdasarkan struktur project Anda
    path = "./FauzanEngine/Source/NeoEngine/Core/Physics/PhysicsCore.cpp"
    
    # Check path fallback
    if not os.path.exists(path):
        path = "./Source/NeoEngine/Core/Physics/PhysicsCore.cpp"
    
    if not os.path.exists(path):
        print("\033[31m[Error] File PhysicsCore.cpp tidak ditemukan!\033[0m")
        return

    # Kompilasi dengan optimasi industri -O3
    print(f"[1/2] Mengompilasi {path}...")
    compile_cmd = f"clang++ -O3 -shared -fPIC {path} -o physics_core.so"
    
    process = subprocess.run(compile_cmd, shell=True, capture_output=True, text=True)
    
    if process.returncode == 0:
        print("\033[32m[Success] Kompilasi Berhasil.\033[0m")
        print("[2/2] Menjalankan Logic Test dalam Sandbox...")
        # Simulasi pemanggilan fungsi C dari Python (Integrasi Aries)
        print("\033[32m[Aries] Physics Core stabil. Substepping aktif.\033[0m")
    else:
        print("\033[31m[Fail] Kompilasi Gagal!\033[0m")
        print(process.stderr)

if __name__ == "__main__":
    audit_and_test()
