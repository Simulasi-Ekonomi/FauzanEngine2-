#!/usr/bin/env python3
import sys

def find_gem(task):
    # Logika sederhana untuk memberikan rekomendasi tool legendaris
    gems = {
        "video": "yt-dlp atau FFmpeg",
        "audio": "FFmpeg",
        "image": "ImageMagick",
        "pdf": "PyMuPDF atau pdf-lib",
        "scraping": "Playwright atau Puppeteer"
    }
    
    result = gems.get(task.lower(), "Cari di GitHub: 'github " + task + " tool'")
    print(f"💎 Gem Recommendation for {task}: {result}")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        find_gem(sys.argv[1])
    else:
        print("Usage: python3 gem_scout.py <task_name>")
