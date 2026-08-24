import os
import json
import socket
from http.server import BaseHTTPRequestHandler, HTTPServer

# =================================================================
# Name        : Aries Sovereign Core v3.0 (Active Agent)
# Master      : Dikri Fauzan (FauzanEngine)
# Capabilities: Real-time Refactor, Live Chat, Code Integration
# =================================================================

PROJECT_ROOT = os.getcwd()

class AriesCoreHandler(BaseHTTPRequestHandler):
    def _set_headers(self, status=200):
        self.send_response(status)
        self.send_header('Content-type', 'application/json')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        self.end_headers()

    def do_OPTIONS(self):
        self._set_headers()

    def do_POST(self):
        content_length = int(self.headers['Content-Length'])
        post_data = self.rfile.read(content_length)
        payload = json.loads(post_data)
        
        intent = payload.get('intent', 'chat')
        topic = payload.get('topic', 'General')
        content = payload.get('content', '')

        print(f"\n\033[1;35m[Aries Core] Perintah Diterima: {intent.upper()}\033[0m")
        
        response_msg = ""
        
        if intent == "refactor":
            # Logika perbaikan kode fisik
            target_file = "Source/NeoEngine/Core/Physics/PhysicsCore.cpp"
            os.makedirs(os.path.dirname(target_file), exist_ok=True)
            
            with open(target_file, "w") as f:
                f.write(content)
            
            print(f"\033[1;32m[Success] File {target_file} telah diperbarui dengan ilmu baru!\033[0m")
            response_msg = f"Kode di {target_file} berhasil di-upgrade berdasarkan riset: {topic}"
            
        elif intent == "chat":
            print(f"\033[1;34m[Master @ UI]: {content}\033[0m")
            response_msg = f"Siap Master Dikri. Ilmu tentang '{topic}' telah saya serap ke memori NeoEngine."

        # Kirim balik respon ke UI
        self._set_headers()
        self.wfile.write(json.dumps({
            "status": "ACTIVE",
            "message": response_msg,
            "agent_id": "139-ARIES-PRIME"
        }).encode())

def run(port=8080):
    server_address = ('', port)
    httpd = HTTPServer(server_address, AriesCoreHandler)
    print("\033[1;34m" + "╔" + "═"*58 + "╗")
    print("║             ARIES SOVEREIGN CORE v3.0 - ACTIVE           ║")
    print("║          SYNCHRONIZED WITH FAUZANENGINE NEO              ║")
    print("╚" + "═"*58 + "╝\033[0m")
    httpd.serve_forever()

if __name__ == "__main__":
    run()
