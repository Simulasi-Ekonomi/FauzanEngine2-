#!/bin/bash
clear
echo "==============================================="
echo "   ARIES COMMAND CENTER - NO HARDCORE MODE    "
echo "==============================================="
echo "1. [BRAIN] Diskusi Tanpa Putus (Reasoning)"
echo "2. [EXIT] Selesai"
echo "==============================================="
read -p "Pilih: " c

case $c in
    1) python3 sovereign.sh ;;
    2) exit ;;
esac
