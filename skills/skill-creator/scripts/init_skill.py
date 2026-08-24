#!/usr/bin/env python3
import sys
import os
from pathlib import Path

# VERSI OTOMATIS: Mencari folder skills relatif dari lokasi script ini
# Script ada di: .../skills/skill-creator/scripts/init_skill.py
# Kita naik 2 tingkat untuk sampai ke folder 'skills'
SKILLS_BASE_PATH = Path(__file__).resolve().parent.parent.parent

SKILL_TEMPLATE = """---
name: {skill_name}
description: [TODO: Deskripsi skill untuk {skill_name}]
---
# {skill_title}
## Overview
[TODO: Penjelasan skill ini]
"""

def init_skill(skill_name):
    skill_dir = SKILLS_BASE_PATH / skill_name
    if skill_dir.exists():
        print(f"❌ Error: Folder {skill_name} sudah ada!")
        return

    try:
        # Membuat folder utama dan subfolder
        os.makedirs(skill_dir / "scripts", exist_ok=True)
        os.makedirs(skill_dir / "references", exist_ok=True)
        os.makedirs(skill_dir / "templates", exist_ok=True)

        # Membuat file SKILL.md
        skill_md = skill_dir / "SKILL.md"
        title = skill_name.replace('-', ' ').title()
        skill_md.write_text(SKILL_TEMPLATE.format(skill_name=skill_name, skill_title=title))
        
        print(f"🚀 Initializing skill: {skill_name}")
        print(f"📍 Location: {skill_dir}")
        print(f"✅ Skill '{skill_name}' berhasil dibuat di folder FauzanEngine!")
    except Exception as e:
        print(f"❌ Error: {e}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 init_skill.py <name>")
        sys.exit(1)
    init_skill(sys.argv[1])
