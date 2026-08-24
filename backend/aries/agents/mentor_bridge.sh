#!/bin/bash
# FAUZAN ENGINE - Agent Mentor Bridge
# Fixed: Menggunakan $(pwd) dan Nama Engine yang Benar

LEVEL=$1
# Memanggil agent mentor dengan path dinamis yang valid
python3 "$(pwd)/backend/aries/agents/agent_mentor.py" $LEVEL

echo "[BRIDGE] Ilmu C++ Level $LEVEL telah di-inject ke FauzanEngine/Source"
