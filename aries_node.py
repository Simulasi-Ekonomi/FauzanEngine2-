import os, json, subprocess, requests
from http.server import BaseHTTPRequestHandler, HTTPServer

class AriesHandler(BaseHTTPRequestHandler):
    def _set_headers(self):
        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        self.end_headers()

    def do_OPTIONS(self): self._set_headers()

    def do_POST(self):
        content_length = int(self.headers['Content-Length'])
        data = json.loads(self.rfile.read(content_length))
        cmd = data.get('cmd')
        path = data.get('path', '')
        content = data.get('content', '')
        
        result = {"status": "success", "output": ""}
        
        try:
            if cmd == "scan":
                findings = []
                for root, _, files in os.walk("."):
                    for f in files:
                        if f.endswith((".cpp", ".h")):
                            p = os.path.join(root, f)
                            with open(p, 'r') as file:
                                if "strcpy" in file.read(): findings.append(p)
                result["output"] = findings
            
            elif cmd == "write":
                os.makedirs(os.path.dirname(path), exist_ok=True)
                with open(path, 'w') as f: f.write(content)
                result["output"] = f"File {path} berhasil diperbarui."
                
            elif cmd == "exec":
                proc = subprocess.run(content, shell=True, capture_output=True, text=True)
                result["output"] = proc.stdout if proc.returncode == 0 else proc.stderr

        except Exception as e:
            result = {"status": "error", "output": str(e)}

        self._set_headers()
        self.wfile.write(json.dumps(result).encode())

print("\033[1;35m[Aries Bridge] Jembatan Aktif. Menunggu Perintah Master dari UI...\033[0m")
HTTPServer(('0.0.0.0', 8080), AriesHandler).serve_forever()
