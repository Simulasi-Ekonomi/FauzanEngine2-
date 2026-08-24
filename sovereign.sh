#!/bin/bash
# NeoEngine Sovereign Command Center

clear
echo -e "\033[1;32m==============================================="
echo -e "      ARIES SOVEREIGN BRAIN v3.5 (S7)"
echo -e "      NEOENGINE COMMAND INTERFACE"
echo -e "===============================================\033[0m"

if [ -z "$1" ]; then
    echo -e "\033[1;33mUsage: ./sovereign.sh \"perintah anda\"\033[0m"
    exit 1
fi

python3 processor.py "$1"
