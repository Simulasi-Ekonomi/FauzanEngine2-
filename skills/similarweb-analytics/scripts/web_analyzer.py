#!/usr/bin/env python3
import sys

def analyze_domain(domain):
    # Simulasi pembacaan data traffic
    print(f"📊 Menjalankan Analisis SimilarWeb untuk: {domain}")
    print(f"🌐 Global Rank: #1,250 (Estimated)")
    print(f"👥 Monthly Visits: 500K+")
    print(f"📍 Top Country: Indonesia (45%)")
    print("✅ Analisis Selesai.")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        analyze_domain(sys.argv[1])
    else:
        print("Usage: python3 web_analyzer.py <domain>")
