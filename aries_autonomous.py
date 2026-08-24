import os
import json
import socket
from http.server import BaseHTTPRequestHandler, HTTPServer

# =================================================================
# Name        : Aries Autonomous Agent v4.0 (Devin Mode)
# Master      : Dikri Fauzan (FauzanEngine Sovereign)
# Capability  : File Scanning, Code Analysis, Auto-Patching
# =================================================================

class AriesAutonomousHandler(BaseHTTPRequestHandler):
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
        
        intent = data.get('intent')
        response_data = {}

        if intent == "scan":
            # Mencari file .cpp dan .h di FauzanEngine
            files_found = []
            for root, dirs, files in os.walk("."):
                for file in files:
                    if file.endswith((".cpp", ".h")):
                        files_found.append(os.path.join(root, file))
            
            # Baca satu file utama untuk dianalisis (Contoh PhysicsCore)
            sample_path = "Source/NeoEngine/Core/Physics/PhysicsCore.cpp"
            content = ""
            if os.path.exists(sample_path):
                with open(sample_path, 'r') as f:
                    content = f.read()
            
            print(f"\033[1;36m[Aries Scan] Master meminta scan. {len(files_found)} file terdeteksi.\033[0m")
            response_data = {
                "status": "SCAN_COMPLETE",
                "files": files_found,
                "current_code": content,
                "target": sample_path
            }

        elif intent == "apply_patch":
            path = data.get('path')
            new_code = data.get('new_code')
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, 'w') as f:
                f.write(new_code)
            
            print(f"\033[1;32m[Aries Patch] Kode di {path} telah diperbaiki sesuai standar industri.\033[0m")
            response_data = {"status": "PATCHED", "message": f"Successfully updated {path}"}

        self._set_headers()
        self.wfile.write(json.dumps(response_data).encode())

def run(port=8080):
    server_address = ('', port)
    httpd = HTTPServer(server_address, AriesAutonomousHandler)
    print("\033[1;34m╔" + "═"*50 + "╗")
    print("║      ARIES AUTONOMOUS AGENT v4.0 (DEVIN MODE)    ║")
    print("║          FAUZANENGINE NEO CORE SYSTEM            ║")
    print("╚" + "═"*50 + "╝\033[0m")
    httpd.serve_forever()

if __name__ == "__main__":
    run()
