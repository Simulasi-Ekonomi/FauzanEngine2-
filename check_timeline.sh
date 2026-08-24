#!/bin/bash

echo "=========================================================="
echo "🕵️ FORENSIK TIMELINE FAUZAN ENGINE (25-26 APRIL)"
echo "=========================================================="

# 1. Cari file yang dimodifikasi dalam rentang waktu Batch Gahar
echo -e "\n[1] File 'Otot' yang dibuat/diubah (Tgl 25 Jam 21:00 - Tgl 26 Jam 02:00):"
find . -type f -newermt "2026-04-25 21:00:00" ! -newermt "2026-04-26 02:00:00" \
    -not -path "*/.git/*" -not -path "*/build/*" \
    -exec ls -l --time-style="+%Y-%m-%d %H:%M" {} + | sort -k6,7

# 2. Cek duplikasi file kritis (Nyari tau siapa yang 'palsu')
echo -e "\n[2] Lokasi Ganda File Kritis (Check for Shadow Files):"
critical_files=("AIControllerCore.cpp" "Engine.cpp" "Transform.cpp" "PrivateAPISystem.h")
for file in "${critical_files[@]}"; do
    echo "--- Checking: $file ---"
    find . -name "$file" -not -path "*/.git/*" -exec ls -l --time-style="+%Y-%m-%d %H:%M" {} +
done

# 3. Scan 'Isi Otot' (NEON & PrivateAPI)
echo -e "\n[3] Audit Konten: Siapa yang beneran Gahar?"
echo "Mencari 'arm_neon' di seluruh Source:"
grep -r "arm_neon" . --exclude-dir={.git,build,node_modules}

echo -e "\nChecking for PrivateAPI implementation:"
grep -r "PrivateAPISystem" . --exclude-dir={.git,build,node_modules}

# 4. Ringkasan Struktur Folder
echo -e "\n[4] Struktur Folder Saat Ini:"
find . -maxdepth 4 -type d -not -path "*/.git/*"
