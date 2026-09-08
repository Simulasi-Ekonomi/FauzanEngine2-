import re

with open("detailed_diff.log", "r") as f:
    content = f.read()

blocks = content.split("==================================================")

print(f"Total blok diperiksa: {len(blocks) - 1}\n")

for b in blocks:
    if not b.strip():
        continue
    file_match = re.search(r"FILE:\s*(.+)", b)
    path_a_match = re.search(r"Path A:\s*(.+)", b)
    path_b_match = re.search(r"Path B:\s*(.+)", b)
    
    if file_match and path_a_match and path_b_match:
        filename = file_match.group(1).strip()
        path_a = path_a_match.group(1).strip()
        path_b = path_b_match.group(1).strip()
        
        # Hitung jumlah baris di diff
        lines_a = len(re.findall(r"^\-", b, re.MULTILINE))
        lines_b = len(re.findall(r"^\+", b, re.MULTILINE))
        
        print(f"[{filename}]")
        print(f"  Path A ({lines_a} lines deleted/changed): {path_a}")
        print(f"  Path B ({lines_b} lines added/changed)  : {path_b}")
        
        # Heuristik kriteria kode produksi
        if lines_b > lines_a + 10:
            print(f"  -> REKOMENDASI: Path B memiliki implementasi lebih lengkap.")
        elif lines_a > lines_b + 10:
            print(f"  -> REKOMENDASI: Path A memiliki implementasi lebih lengkap.")
        else:
            print(f"  -> PERLU AUDIT MANUAL: Ukuran implementasi mirip.")
        print("-" * 60)
