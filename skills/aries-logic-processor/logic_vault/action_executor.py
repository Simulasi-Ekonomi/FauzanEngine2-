import os
import re
import shutil

class ActionExecutorSkill:
    def execute(self, mem_path=None):
        target_dir = os.getcwd()
        log = ["#### ⚡ ARIES V6 FIXED: INTELLIGENT PATH RECOVERY"]
        fixed_count = 0

        # Pattern untuk menangkap path hardcoded FauzanEngine
        broad_path_pattern = r"\"/sdcard/Buku saya/FauzanEngine[^\"]*\""
        
        for root, dirs, files in os.walk(target_dir):
            # Skip folder sampah/sistem
            if any(x in root for x in ['aries-logic-processor', '.git', '__pycache__']):
                continue
                
            for file in files:
                # HANYA PROSES FILE PYTHON (.py)
                if file.endswith('.py'):
                    path = os.path.join(root, file)
                    try:
                        with open(path, 'r', errors='ignore') as f:
                            content = f.read()
                        
                        original = content
                        
                        # 1. Ganti path statis jadi os.getcwd()
                        if re.search(broad_path_pattern, content):
                            content = re.sub(broad_path_pattern, "os.getcwd()", content)

                        # 2. Perbaiki error handling yang malas
                        content = content.replace("except: pass", "except Exception as e: print(f'Logging: {e}')")

                        if content != original:
                            # Simpan backup sebelum modifikasi
                            shutil.copy2(path, path + ".bak")
                            with open(path, 'w') as f:
                                f.write(content)
                            fixed_count += 1
                    except Exception as e:
                        continue
                
                # JANGAN EDIT FILE .CPP, .SH, .XML (Abaikan saja)
                else:
                    continue

        log.append(f"- `[FIXED]` Total {fixed_count} file Python berhasil didinamiskan.")
        log.append("- `[SAFE]` File sistem (.sh, .cpp, .xml) tidak disentuh untuk mencegah crash.")
        return "\n".join(log)
