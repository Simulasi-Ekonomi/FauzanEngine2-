import json
import os
import sys
import urllib.request
from pathlib import Path

SKILLS_DIR = Path("/sdcard/Buku saya/FauzanEngine/skills")
CACHE_FILE = SKILLS_DIR / "internet-skill-finder" / "references" / "skills_cache.json"

def try_download(url):
    try:
        req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
        with urllib.request.urlopen(req) as response:
            return response.read().decode('utf-8')
    except:
        return None

def import_skill(skill_name):
    with open(CACHE_FILE, 'r') as f:
        data = json.load(f)
        target_url = None
        for repo, content in data.items():
            for skill in content.get("skills", []):
                if skill['name'].lower() == skill_name.lower():
                    target_url = skill.get("github_url")
                    repo_name = repo
                    break
        
        if not target_url: return f"❌ Skill {skill_name} tidak ditemukan."

        base_raw = target_url.replace("github.com", "raw.githubusercontent.com").replace("/blob/", "/").replace("/tree/", "/")
        
        # Daftar kemungkinan path file asli di GitHub
        candidates = [
            base_raw + "/SKILL.md",
            base_raw.rstrip("/") + "/SKILL.md",
            base_raw + "/skills/" + skill_name + "/SKILL.md",
            base_raw.replace("/main/", "/master/") + "/SKILL.md"
        ]

        content = None
        for url in candidates:
            print(f"📡 Mencoba akses: {url}")
            content = try_download(url)
            if content: break
        
        if content:
            # Normalisasi nama folder agar tidak ada spasi
            folder_name = skill_name.lower().replace(" ", "-")
            skill_path = SKILLS_DIR / folder_name
            os.makedirs(skill_path, exist_ok=True)
            with open(skill_path / "SKILL.md", "w") as f:
                f.write(content)
            return f"✅ Berhasil! File asli '{skill_name}' tersimpan di: {folder_name}/SKILL.md"
        
        return f"❌ Gagal mengambil file asli untuk {skill_name}. File mungkin tidak ada di Repo."

if __name__ == "__main__":
    if len(sys.argv) > 1:
        print(import_skill(" ".join(sys.argv[1:])))
