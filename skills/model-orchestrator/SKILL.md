---
name: model-orchestrator
description: Central logic for routing tasks between local LLMs (Gemma4, Hermes, Ruflo). Specialized in context-switching and model delegation.
---
# Local Model Orchestrator

## 🤖 Model Routing Logic
- **Gemma4 (Main Brain)**: Gunakan untuk logika arsitektur engine dan pengambilan keputusan Sovereign.
- **Hermes (Creative/Chat)**: Gunakan untuk penulisan narasi, instruksi user-friendly, dan brainstorming ide baru.
- **Ruflo (Performance)**: Gunakan untuk analisa bottleneck sistem dan optimasi logika berat.

## ⚙️ Protocol
1. Analisa beban tugas.
2. Jika tugas bersifat sistem/fisik, delegasikan ke **Gemma4**.
3. Jika tugas bersifat kreatif, delegasikan ke **Hermes**.
4. Gabungkan hasil kembali ke dalam context utama Aries.
