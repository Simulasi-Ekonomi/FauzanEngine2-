import sys
import json
import os
import asyncio
from openai import OpenAI

client = OpenAI(
    base_url="https://integrate.api.nvidia.com/v1",
    api_key=os.environ.get("FZ_NVIDIA_API_KEY")
)

async def handle_request(prompt: str):
    try:
        completion = client.chat.completions.create(
            model="nvidia/nemotron-3.5-lightning-30b-a3b",
            messages=[{"role": "user", "content": prompt}],
            temperature=1,
            top_p=0.95,
            max_tokens=16384,
            extra_body={"chat_template_kwargs": {"enable_thinking": True}, "reasoning_budget": 16384},
            stream=True
        )

        full_response = ""
        for chunk in completion:
            if not chunk.choices:
                continue
            reasoning = getattr(chunk.choices[0].delta, "reasoning_content", None)
            if reasoning:
                sys.stderr.write(reasoning)
                sys.stderr.flush()
            if chunk.choices[0].delta.content is not None:
                content = chunk.choices[0].delta.content
                full_response += content
        return full_response
    except Exception as e:
        sys.stderr.write(f"Error: {str(e)}\n")
        sys.stderr.flush()
        return None

async def main():
    loop = asyncio.get_event_loop()
    while True:
        line = await loop.run_in_executor(None, sys.stdin.readline)
        if not line:
            break
        try:
            request = json.loads(line)
            method = request.get("method")
            req_id = request.get("id")

            if method == "initialize":
                response = {
                    "jsonrpc": "2.0",
                    "id": req_id,
                    "result": {
                        "protocolVersion": "2024-11-05",
                        "capabilities": {"tools": {}},
                        "serverInfo": {"name": "nemotron-mcp-server", "version": "1.2.0"}
                    }
                }
                sys.stdout.write(json.dumps(response) + "\n")
                sys.stdout.flush()

            elif method == "notifications/initialized":
                continue

            elif method == "tools/list":
                response = {
                    "jsonrpc": "2.0",
                    "id": req_id,
                    "result": {
                        "tools": [{
                            "name": "ask_nemotron",
                            "description": "Kirim prompt ke model NVIDIA Nemotron 3.5 Lightning menggunakan FZ_NVIDIA_API_KEY.",
                            "inputSchema": {
                                "type": "object",
                                "properties": {
                                    "prompt": {"type": "string", "description": "Pertanyaan atau kode yang ingin dianalisis"}
                                },
                                "required": ["prompt"]
                            }
                        }]
                    }
                }
                sys.stdout.write(json.dumps(response) + "\n")
                sys.stdout.flush()

            elif method == "tools/call":
                params = request.get("params", {})
                if params.get("name") == "ask_nemotron":
                    prompt_arg = params.get("arguments", {}).get("prompt", "")
                    result_text = await handle_request(prompt_arg)
                    response = {
                        "jsonrpc": "2.0",
                        "id": req_id,
                        "result": {
                            "content": [{"type": "text", "text": result_text if result_text else "Error executing model."}]
                        }
                    }
                    sys.stdout.write(json.dumps(response) + "\n")
                    sys.stdout.flush()
        except json.JSONDecodeError:
            continue
        except Exception as ex:
            sys.stderr.write(f"Protocol Error: {str(ex)}\n")
            sys.stderr.flush()

if __name__ == "__main__":
    asyncio.run(main())
