#!/bin/bash

SOURCE_DIR="engine/Source/NeoEngine"
echo -e "\033[1;34m=== FAUZANENGINE AUDIT REPORT (FIXED) ===\033[0m"

# Hitung File
TOTAL_FILES=$(find $SOURCE_DIR -name "*.h" -o -name "*.cpp" | wc -l)
echo "Total Files Scanned: $TOTAL_FILES"
echo "--------------------------------------"

# Cari List
STUBS=$(grep -rnE "STUB|Placeholder|// Implement|// TODO" $SOURCE_DIR)

# Hitung Akurat
STUB_COUNT=$(echo "$STUBS" | grep -E "STUB|Placeholder" | wc -l)
TODO_COUNT=$(echo "$STUBS" | grep "TODO" | wc -l)

# Tampilkan Lokasi
echo "$STUBS" | awk -F: '{print "📍 File: " $1 " (Line " $2 ")"}'

echo "--------------------------------------"
echo -e "\033[1;32mAUDIT SUMMARY:\033[0m"
echo -e "🔴 Stubs Found : $STUB_COUNT"
echo -e "🟡 TODOs Found : $TODO_COUNT"

if [ $STUB_COUNT -eq 0 ]; then
    echo -e "\n\033[1;42m STATUS: 100% SOLID \033[0m"
else
    # Hitung persentase real
    STUB_VAL=$STUB_COUNT
    TOTAL_VAL=$TOTAL_FILES
    RATE=$(awk "BEGIN {print (1 - ($STUB_VAL / $TOTAL_VAL)) * 100}")
    echo -e "\n\033[1;44m STATUS: DEVELOPMENT IN PROGRESS (${RATE:0:5}%) \033[0m"
fi
