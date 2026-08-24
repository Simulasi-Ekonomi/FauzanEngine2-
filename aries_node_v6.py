import os
import json
import subprocess
from http.server import BaseHTTPRequestHandler, HTTPServer

# =================================================================
# Name        : Aries Terminal Node v6.0 (Passive Executor)
# Description : Deep Integration for FauzanEngine (Devin Mode)
# Master      : Dikri Fauzan
# =================================================================

class AriesNodeHandler(BaseHTTPRequestHandler):
    def _set_headers(self):
        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        self.end_headers()

    def do_OPTIONS(self):
        self._set_headers()

    def do_POST(self):
        content_length = int(self.headers['Content-Length'])
        data = json.loads(self.rfile.read(content_length))
        
        command = data.get('command')
        payload = data.get('payload', {})
        result = {"status": "SUCCESS", "output": ""}

        try:
            if command == "scan_weakness":
                # Melaporkan struktur folder dan mencari indikasi kode lemah
                report = []
                for root, _, filenames in os.walk("."):
                    for f in filenames:
                        if f.endswith((".cpp", ".h")):
                            path = os.path.join(root, f)
                            with open(path, 'r') as file:
                                content = file.read()
                                if "strcpy" in content or "malloc" in content: # Contoh deteksi lemah
                                    report.append({"file": path, "issue": "Legacy memory management detected"})
                result["output"] = report
                print(f"\033[1;33m[Node] Scan Selesai. {len(report)} Isu dilaporkan ke UI.\033[0m")
                
            elif command == "write_gold_code":
                # Menulis kode emas tanpa modifikasi logika, hanya path
                path = payload.get('path')
                content = payload.get('content')
                os.makedirs(os.path.dirname(path), exist_ok=True)
                with open(path, 'w') as f:
                    f.write(content)
                result["output"] = f"Gold Code deployed to {path}"
                print(f"\033[1;32m[Node] Gold Code Applied: {path}\033[0m")
                
            elif command == "run_cmake":
                print("\033[1;36m[Node] Menjalankan CMake Build...\033[0m")
                proc = subprocess.run(["cmake", "."], capture_output=True, text=True)
                result["output"] = proc.stdout if proc.returncode == 0 else proc.stderr

        except Exception as e:
            result = {"status": "ERROR", "output": str(e)}

        self._set_headers()
        self.wfile.write(json.dumps(result).encode())

def run(port=8080):
    server_address = ('', port)
    httpd = HTTPServer(server_address, AriesNodeHandler)
    print("\033[1;34m[Aries v6] Node Standby. Menunggu Pusat Komando Gemini...\033[0m")
    httpd.serve_forever()

if __name__ == "__main__":
    run()
