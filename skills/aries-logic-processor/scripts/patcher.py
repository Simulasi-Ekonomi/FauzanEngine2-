import os

def add_skill(skill_name, code_block):
    file_path = "/sdcard/Buku saya/FauzanEngine/skills/aries-logic-processor/scripts/processor.py"
    
    if not os.path.exists(file_path):
        print("❌ File processor.py tidak ada!")
        return

    with open(file_path, 'r') as f:
        content = f.read()

    # Cek apakah skill sudah ada supaya tidak duplikat
    if skill_name in content:
        print(f"⚠️ Skill {skill_name} sudah terpasang.")
        return

    # Cari baris terakhir sebelum blok 'if __name__ == "__main__":'
    if 'if __name__ == "__main__":' in content:
        parts = content.split('if __name__ == "__main__":')
        new_content = parts[0] + "\n" + code_block + "\n" + 'if __name__ == "__main__":' + parts[1]
        
        with open(file_path, 'w') as f:
            f.write(new_content)
        print(f"✅ Skill {skill_name} berhasil ditambahkan ke otak Aries!")
