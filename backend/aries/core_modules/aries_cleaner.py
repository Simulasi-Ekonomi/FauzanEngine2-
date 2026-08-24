import re

def clean_aries_output_string(text):
    """Fungsi khusus untuk membersihkan output internal Aries"""
    if not text: return ""
    
    # 1. Bersihkan sisa-sisa JSON escape kalau ada
    clean = text.replace('\\\\n', '\n').replace('\\n', '\n')
    clean = clean.replace('\\"', '"').replace("\\'", "'")
    
    # 2. Rapikan Header S8 & V5 (Biar kayak GPT-5.2)
    # Mencari pola [TEXT] atau TEXT: dan menjadikannya Bold/Header
    clean = re.sub(r'(\[.*?\])', r'\n\033[1;36m\1\033[0m', clean) 
    clean = re.sub(r'([A-Z\s]{5,}):', r'\n\033[1;33m\1\033[0m', clean)
    
    # 3. Buat Bullet Points lebih rapi
    clean = clean.replace(' - ', '\n  • ')
    
    # 4. Buat pembatas lebih elegan
    clean = clean.replace('------------------------------------------', '─' * 45)
    
    return clean.strip()

if __name__ == "__main__":
    import sys, json
    # Tetap dukung mode pipe untuk debugging
    raw = sys.stdin.read()
    print(clean_aries_output_string(raw))
