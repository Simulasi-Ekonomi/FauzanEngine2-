import os
from http.server import BaseHTTPRequestHandler, HTTPServer
import json
import socket

class AriesServer(BaseHTTPRequestHandler):
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
        post_data = self.rfile.read(content_length)
        data = json.loads(post_data)

        print("\n\033[1;34m" + "█"*50)
        print(f"\033[1;32m[ARIES RECEIVER] ILMU BARU DITERIMA DARI UI!")
        print(f"\033[1;36mTopik : {data.get('topic', 'N/A')}")
        print("\033[0m" + "-"*50)
        print(f"\033[1;37mContent: {data.get('content')[:500]}...")
        
        # Logika Injeksi Kode ke Engine
        if "cpp" in data.get('topic').lower() or "physics" in data.get('topic').lower():
            print("\033[1;33m[NEO ENGINE] Injeksi ilmu ke Source/Core/Physics/PhysicsCore.cpp...\033[0m")
        
        print("\033[1;34m" + "█"*50 + "\033[0m\n")

        self._set_headers()
        self.wfile.write(json.dumps({"status": "SUCCESS", "message": "Aries Termux Synced"}).encode())

def get_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try: s.connect(('8.8.8.8', 80)); ip = s.getsockname()[0]
    except: ip = '127.0.0.1'
    finally: s.close()
    return ip

def run(port=8080):
    server_address = ('', port)
    httpd = HTTPServer(server_address, AriesServer)
    print("\033[1;34m╔" + "═"*48 + "╗")
    print(f"║   ARIES SOVEREIGN RECEIVER v2.9 - ACTIVE       ║")
    print(f"║   IP TERMUX : {get_ip()}:{port}              ║")
    print("╚" + "═"*48 + "╝\033[0m")
    print("\033[1;33m[Sovereign] Menunggu data dari UI Cloud Bridge...\033[0m")
    httpd.serve_forever()

if __name__ == "__main__":
    run()
