#!/bin/bash

# Warna untuk output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}===============================================${NC}"
echo -e "${YELLOW}       ARIES SOVEREIGN LOGIC AUDIT TOOL        ${NC}"
echo -e "${YELLOW}===============================================${NC}"

BASE_DIR=$(pwd)

# 1. Audit Struktur Folder
echo -e "\n[1/3] Memeriksa Fondasi Fisik..."
folders=("scripts" "references" "templates")
for dir in "${folders[@]}"; do
    if [ -d "$BASE_DIR/$dir" ]; then
        echo -e "  [${GREEN}OK${NC}] Folder $dir ditemukan."
    else
        echo -e "  [${RED}FAIL${NC}] Folder $dir hilang!"
    fi
done

# 2. Audit SKILL.md (Otak Aries)
echo -e "\n[2/3] Memeriksa Kesadaran (SKILL.md)..."
if [ -f "$BASE_DIR/SKILL.md" ]; then
    echo -e "  [${GREEN}OK${NC}] SKILL.md ditemukan."
    
    # Cek apakah ada Workflow (Syarat Anti-Lobotomy)
    if grep -q "Workflow" "$BASE_DIR/SKILL.md"; then
        echo -e "  [${GREEN}OK${NC}] Instruksi Workflow terdeteksi."
    else
        echo -e "  [${YELLOW}WARNING${NC}] SKILL.md tidak punya Workflow! Potensi Lobotomy Tinggi."
    fi
    
    # Cek metadata name
    if grep -q "name: aries-logic-processor" "$BASE_DIR/SKILL.md"; then
         echo -e "  [${GREEN}OK${NC}] Metadata Name Valid."
    fi
else
    echo -e "  [${RED}FAIL${NC}] SKILL.md tidak ditemukan! Aries tidak punya otak."
fi

# 3. Audit Pengetahuan (References)
echo -e "\n[3/3] Memeriksa Bank Data S7..."
refs=("game_logic.md" "engine_core.md")
for file in "${refs[@]}"; do
    if [ -f "$BASE_DIR/references/$file" ]; then
        line_count=$(wc -l < "$BASE_DIR/references/$file")
        echo -e "  [${GREEN}OK${NC}] $file aktif ($line_count baris ilmu)."
    else
        echo -e "  [${RED}FAIL${NC}] $file hilang! Aries akan buta teknis."
    fi
done

echo -e "\n${YELLOW}-----------------------------------------------${NC}"
echo -e "HASIL AKHIR: "
if [ -f "$BASE_DIR/SKILL.md" ] && [ -d "$BASE_DIR/references" ]; then
    echo -e "${GREEN}ARIES SIAP BEROPERASI (MODE REASONING ACTIVE)${NC}"
else
    echo -e "${RED}ARIES CACAT PRODUKSI (LOBOTOMY DETECTED)${NC}"
fi
echo -e "${YELLOW}-----------------------------------------------${NC}"
