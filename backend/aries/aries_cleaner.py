import json
import re
import sys

def clean_aries_output_string(raw_text):
    """Versi fungsi untuk dipanggil internal oleh Aries Brain"""
    try:
        clean_text = raw_text
        try:
            clean_text = clean_text.encode().decode('unicode_escape')
            clean_text = clean_text.encode().decode('unicode_escape')
        except Exception as e: print(f'Logging: {e}')
        clean_text = clean_text.replace('\\\\n', '\n').replace('\\n', '\n')
        clean_text = clean_text.replace('\\"', '"').replace("\\'", "'")
        clean_text = re.sub(r'\n([A-Z\s]{4,}):', r'\n\n### \1\n', clean_text)
        clean_text = re.sub(r'(\d+\.)\s', r'\n\1 ', clean_text)
        
        output = "\n" + "✨" + " ="*25 + "\n"
        output += " 🚀  ARIES INTELLIGENCE - DECODED OUTPUT\n"
        output += " ="*25 + "\n\n"
        output += clean_text.strip()
        output += "\n\n" + " ="*25 + "\n"
        output += " ✅  END OF MESSAGE\n"
        output += " ="*25 + "\n"
        return output
    except:
        return raw_text

def clean_aries_output(raw_data):
    try:
        data = json.loads(raw_data)
        content = data.get('text') or data.get('response') or data.get('prompt') or str(data)
        print(clean_aries_output_string(content))
    except Exception as e:
        print(f"❌ Error saat merapikan output: {e}")
        print(raw_data.replace('\\n', '\n'))

if __name__ == "__main__":
    if not sys.stdin.isatty():
        raw_input = sys.stdin.read()
        if raw_input.strip():
            clean_aries_output(raw_input)
    else:
        print("💡 Cara pakai: curl <url_aries> | python3 aries_cleaner.py")
