import os
import re

class DeepScannerSkill:
    def execute(self, mem_path):
        target_dir = os.getcwd() if "os" in locals() else "."
        report = ["#### 🔍 ARIES S7: ADVANCED DIAGNOSTIC REPORT"]
        
        issues = {"P0": [], "P1": [], "P2": []}
        file_count = 0

        for root, dirs, files in os.walk(target_dir):
            if 'aries-logic-processor' in root or 'node_modules' in root: continue
            
            for file in files:
                file_count += 1
                path = os.path.join(root, file)
                try:
                    with open(path, 'r', errors='ignore') as f:
                        content = f.read()
                        
                        # 🔴 DETEKSI P0: XML Malformed (AndroidManifest)
                        if file == "AndroidManifest.xml":
                            if re.search(r"<application[^>]*\n\s*<uses-native-library", content):
                                issues["P0"].append(f"Malformed XML in {file}: Tag child muncul sebelum atribut <application> selesai.")

                        # 🔴 DETEKSI P0: CMake Linking (libcurl)
                        if file == "CMakeLists.txt":
                            if "find_library(curl-lib curl)" in content and "ExternalProject" not in content:
                                issues["P0"].append(f"Potential Link Error in {file}: libcurl tidak tersedia default di NDK.")

                        # 🔴 DETEKSI P0: Hardcoded /sdcard/
                        if "/sdcard/" in content and not file.endswith(".txt"):
                            issues["P0"].append(f"Hardcoded Path in {file}: Menggunakan /sdcard/ (Tidak Portable).")

                        # 🟡 DETEKSI P1: Race Condition / Silent Failure
                        if "except:" in content and "pass" in content:
                            issues["P1"].append(f"Silent Failure in {file}: Blok except: pass terdeteksi (Data loss risk).")
                        
                        if "popleft()" in content and "Lock()" not in content:
                            issues["P1"].append(f"Race Condition in {file}: deque.popleft() tanpa threading lock.")

                except: continue

        # Menyusun Report
        report.append(f"**Total File Dipindai**: {file_count}")
        for priority in ["P0", "P1", "P2"]:
            if issues[priority]:
                report.append(f"\n**[{priority}] CRITICAL ISSUES:**")
                report.append("\n".join(issues[priority][:15]))

        return "\n".join(report)
