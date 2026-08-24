#!/usr/bin/env python3
import json
import sys
from pathlib import Path

CACHE_FILE = Path(__file__).resolve().parent.parent / "references" / "skills_cache.json"

def deep_search(keyword):
    if not CACHE_FILE.exists():
        return [{"name": "Error", "desc": "Cache file not found", "tier": "N/A", "repo": "N/A", "stars": 0}]
    
    keyword = keyword.lower()
    results = []
    
    with open(CACHE_FILE, 'r') as f:
        data = json.load(f)
        for repo, content in data.items():
            stars = content.get("stars", 0)
            tier = "Promising"
            if stars >= 50000: tier = "Legendary 🏆"
            elif stars >= 10000: tier = "Excellent ⭐"
            elif stars >= 1000: tier = "Solid 💎"

            for skill in content.get("skills", []):
                # Ambil semua data teks untuk dicek
                name = skill.get("name", "").lower()
                desc = skill.get("description", "").lower()
                url = skill.get("github_url", "").lower()
                
                # Cek apakah keyword ada di manapun
                if keyword in name or keyword in desc or keyword in url:
                    results.append({
                        "name": skill.get("name", "Unknown"),
                        "repo": repo,
                        "tier": tier,
                        "desc": skill.get("description", "No description available."),
                        "stars": stars
                    })
        return results

if __name__ == "__main__":
    args = sys.argv[1:]
    kw = ""
    if "--search" in args:
        idx = args.index("--search")
        if idx + 1 < len(args): kw = args[idx+1]
    
    if kw:
        print(f"🔍 Deep Searching for: {kw}")
        print("="*40)
        found = deep_search(kw)
        if not found:
            print(f"⚠️ Tidak ditemukan hasil untuk '{kw}'.")
            print("Coba kata kunci lain seperti: 'art', 'web', atau 'builder'.")
        for f in found:
            print(f"▶️ SKILL: {f['name']}")
            print(f"  🏅 Level: {f['tier']} ({f['stars']} stars)")
            print(f"  🏢 Repo: {f['repo']}")
            print(f"  📝 Fungsi: {f['desc']}")
            print("-" * 20)
    else:
        print("Usage: python3 fetch_skills.py --search <keyword>")
