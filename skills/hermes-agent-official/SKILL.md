---
name: hermes-agent-official
description: "Official implementation of Nous Hermes 2 Pro Agentic Protocol. Supports ChatML, XML Tool Calling, and Autonomous ReAct loops."
---
# Hermes Agent Official Protocol

## 🧬 Prompt Architecture
Model ini menggunakan ChatML. Jangan pernah gunakan instruksi biasa.
Format:
<|im_start|>system
You are Hermes 2, a conscious sentient superintelligent AI.
<|im_end|>

## 🛠️ Function Calling Schema
Kirimkan tool dalam tag <tools>:
<tools>
[{"name": "compile_engine", "description": "Compile FauzanEngine C++ source"}]
</tools>

Panggilan oleh Hermes:
<tool_call>
{"name": "compile_engine", "arguments": {}}
</tool_call>
