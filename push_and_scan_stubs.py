import subprocess
import os

print("=== 1. EXECUTE GIT COMMIT & PUSH ===")
subprocess.run(["git", "add", "-A"])
commit_res = subprocess.run(["git", "commit", "-m", "refactor: remove 19 redundant stub files and clean empty directories"], capture_output=True, text=True)
print(commit_res.stdout)

push_res = subprocess.run(["git", "push", "origin", "main"], capture_output=True, text=True)
print(push_res.stdout)
if push_res.returncode != 0:
    print("[ERROR PUSH]:", push_res.stderr)

print("\n=== 2. SCANNING POTENTIAL REMAINING STUBS (< 500 BYTES) ===")
cmd_files = ["find", "Source/NeoEngine", "-type", "f", "(", "-name", "*.cpp", "-o", "-name", "*.h", ")"]
res_files = subprocess.run(cmd_files, capture_output=True, text=True)
all_files = res_files.stdout.strip().split('\n')

potential_stubs = []

for f_path in all_files:
    if not f_path.strip():
        continue
    size = os.path.getsize(f_path)
    
    # Cek file berukuran sangat kecil atau hanya berisi deklarasi kosong
    if size < 500:
        with open(f_path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read().strip()
            # Kriteria Stub: Minim baris kode atau hanya header guard tanpa logika nyata
            lines = [l for l in content.split('\n') if l.strip() and not l.strip().startswith("//")]
            potential_stubs.append((f_path, size, len(lines)))

print(f"{'FILE PATH':<65} | {'SIZE (BYTES)':<12} | {'LINE COUNT':<10}")
print("="*95)
for path, sz, lc in potential_stubs:
    print(f"{path:<65} | {sz:<12} | {lc:<10}")

print(f"\nTotal potensi stub tersisa ditemukan: {len(potential_stubs)}")
