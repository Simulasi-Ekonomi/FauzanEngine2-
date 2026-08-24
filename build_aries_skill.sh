#!/bin/bash

# 1. Tentukan Path
BASE_DIR=$(pwd)

# 2. Buat Folder secara rekursif
mkdir -p "$BASE_DIR/scripts"
mkdir -p "$BASE_DIR/references"
mkdir -p "$BASE_DIR/templates"

echo "✅ Folder Skill Aries Terbentuk."

# 3. Tulis SKILL.md (Instruksi Utama Aries)
cat << 'SKILL_EOF' > "$BASE_DIR/SKILL.md"
---
name: aries-logic-processor
description: Spesialis pengembang game FauzanEngine. Mengolah ilmu S7 menjadi workflow nyata. Gunakan untuk: perencanaan mekanik, naskah, dan world building.
---

# Aries Logic Processor

## Workflow Anti-Lobotomy
Aries wajib menjalankan tahapan ini:
1. **Analisis Prompt**: Bedah apa mau Bos (Sudoku? TD? RPG?).
2. **Consult Reference**: Hanya baca file di `references/` yang sesuai.
3. **Drafting**: Tulis naskah dan perencanaan teknis secara manusiawi.
4. **Logic Check**: Hubungkan ke fitur FauzanEngine (AABB, MemoryGuard).

## Referensi
- [Engine Core](references/engine_core.md)
- [Game Design](references/game_design.md)
- [Asset Standards](references/art_mastery.md)
SKILL_EOF

echo "✅ SKILL.md Berhasil Ditulis."

# 4. Tulis Referensi Awal (Memecah data 10k Bos)
cat << 'REF_EOF' > "$BASE_DIR/references/game_design.md"
# Game Design Reference
- Sudoku: Logic-based grid, no physics, matrix validation.
- Tower Defense: Pathfinding level 6, object pooling level 8.
- RPG: State machine level 6, world generate PCK streaming.
REF_EOF

echo "✅ File Referensi Terbuat."
