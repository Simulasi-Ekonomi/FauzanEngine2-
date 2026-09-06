#!/usr/bin/env python3
import json
import os
import subprocess
import sys

MEMORY_FILE = os.path.expanduser("~/.jagoan_memory.json")

def load_memory():
    if os.path.exists(MEMORY_FILE):
        try:
            with open(MEMORY_FILE, "r", encoding="utf-8") as f:
                return json.load(f)
        except Exception:
            pass
    return {"user_preferences": {}, "learned_patterns": [], "session_history": []}

def save_memory(memory_data):
    try:
        memory_data["session_history"] = memory_data["session_history"][-20:]
        with open(MEMORY_FILE, "w", encoding="utf-8") as f:
            json.dump(memory_data, f, indent=2, ensure_ascii=False)
    except Exception as e:
        print(f"Warning: Gagal menyimpan memori: {e}", file=sys.stderr)

def main():
    if len(sys.argv) < 2:
        print("Error: Prompt file argument missing", file=sys.stderr)
        sys.exit(1)

    prompt_file_path = sys.argv[1]
    if not os.path.exists(prompt_file_path):
        print(f"Error: File {prompt_file_path} tidak ditemukan", file=sys.stderr)
        sys.exit(1)

    with open(prompt_file_path, "r", encoding="utf-8") as f:
        user_prompt_content = f.read()

    memory = load_memory()
    
    memory_context = "\n==================================================\n"
    memory_context += "INGATAN DAN MEMORI PERCAKAPAN TERSIMPAN (MACHINE LEARNING STORE):\n"
    memory_context += f"Preferensi Pengguna: {json.dumps(memory['user_preferences'], ensure_ascii=False)}\n"
    memory_context += f"Pola Terpelajari (Learned Patterns):\n"
    for idx, pattern in enumerate(memory["learned_patterns"], 1):
        memory_context += f"  {idx}. {pattern}\n"
    
    if memory["session_history"]:
        memory_context += f"\nRiwayat Percakapan Terakhir ({len(memory['session_history'])} interaksi):\n"
        for item in memory["session_history"][-5:]:
            memory_context += f"- User: {item.get('prompt_summary', '')}\n  Copilot: {item.get('response_summary', '')}\n"
    memory_context += "==================================================\n\n"

    final_payload = memory_context + user_prompt_content

    script_dir = os.path.dirname(os.path.abspath(__file__))
    server_script = os.path.join(script_dir, "nemotron_mcp_server.py")

    process = subprocess.Popen(
        [sys.executable, server_script],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        env=os.environ
    )

    init_req = {
        "jsonrpc": "2.0",
        "id": 1,
        "method": "initialize",
        "params": {
            "protocolVersion": "2024-11-05",
            "capabilities": {},
            "clientInfo": {"name": "JagoanMemoryEngine", "version": "4.1"}
        }
    }
    process.stdin.write(json.dumps(init_req) + "\n")
    process.stdin.flush()
    _ = process.stdout.readline()

    call_req = {
        "jsonrpc": "2.0",
        "id": 2,
        "method": "tools/call",
        "params": {
            "name": "ask_nemotron",
            "arguments": {"prompt": final_payload}
        }
    }
    process.stdin.write(json.dumps(call_req) + "\n")
    process.stdin.flush()

    stdout_call = process.stdout.readline()
    process.terminate()

    try:
        res = json.loads(stdout_call)
        if "result" in res and "content" in res["result"]:
            output_text = res["result"]["content"][0]["text"]
            print(output_text)

            prompt_summary = user_prompt_content.splitlines()[-1] if user_prompt_content.splitlines() else "Command"
            resp_summary = output_text.splitlines()[0] if output_text.splitlines() else "Response"
            
            memory["session_history"].append({
                "prompt_summary": prompt_summary[:150],
                "response_summary": resp_summary[:150]
            })
            save_memory(memory)

        elif "error" in res:
            print(f"MCP Error: {res['error']}", file=sys.stderr)
            sys.exit(1)
        else:
            print(f"Response Tidak Valid: {stdout_call}", file=sys.stderr)
            sys.exit(1)
    except json.JSONDecodeError:
        print(f"Gagal parse respon JSON: {stdout_call}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
