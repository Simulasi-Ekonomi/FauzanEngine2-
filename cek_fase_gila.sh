#!/bin/bash

echo "=========================================================="
echo "🕵️ AUDIT TIMELINE: FASE PRODUKSI MASSAL (23:00 - 13:30)"
echo "=========================================================="

# Mencari file yang dibuat/diubah tepat di jam kerja lo (25 Apr 23:00 s/d 26 Apr 13:30)
# -not -path digunakan agar tidak terganggu file dari node_modules atau .git
find . -type f -newermt "2026-04-25 23:00:00" ! -newermt "2026-04-26 13:30:00" \
    -not -path "*/.git/*" \
    -not -path "*/node_modules/*" \
    -not -path "*/.gradle/*" \
    -exec ls -l --time-style="+%d %b %H:%M" {} + | sort -k6,7

echo -e "\n----------------------------------------------------------"
echo "📊 RINGKASAN JUMLAH FILE DI FASE INI:"
find . -type f -newermt "2026-04-25 23:00:00" ! -newermt "2026-04-26 13:30:00" \
    -not -path "*/.git/*" -not -path "*/node_modules/*" | wc -l
echo "----------------------------------------------------------"
