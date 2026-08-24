import requests
import json

def ask_hermes(prompt, system_prompt="You are a Sovereign AI Agent."):
    url = "http://127.0.0.1:11434/api/generate"
    payload = {
        "model": "hermes2pro",
        "prompt": f"<|im_start|>system\n{system_prompt}<|im_end|>\n<|im_start|>user\n{prompt}<|im_end|>\n<|im_start|>assistant",
        "stream": False
    }
    try:
        response = requests.post(url, json=payload)
        return response.json().get('response', 'Error: No response')
    except Exception as e:
        return f"Connection Error: {str(e)}"

if __name__ == "__main__":
    print("Testing Bridge to Hermes...")
    # Script ini akan ready setelah download selesai
