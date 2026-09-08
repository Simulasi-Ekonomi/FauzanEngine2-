#!/bin/bash
echo "=================================================="
echo "          NEOENGINE BUILD ARTIFACT AUDIT          "
echo "=================================================="

BUILD_DIR="build"

if [ ! -d "$BUILD_DIR" ]; then
    echo "[!] ERR: Direktori $BUILD_DIR tidak ditemukan!"
    exit 1
fi

echo -e "\n[1] EXECUTABLES PRODUCED:"
find "$BUILD_DIR" -maxdepth 2 -type f -executable ! -name "*.so" ! -name "*.sh" -exec ls -lh {} + 2>/dev/null | awk '{print $9, "(Size: " $5 ")"}'

echo -e "\n[2] SMOKE TEST EXECUTION CHECK:"
if [ -f "$BUILD_DIR/NeoEngineSmokeTest" ]; then
    echo "[+] Running NeoEngineSmokeTest..."
    ./"$BUILD_DIR"/NeoEngineSmokeTest
else
    echo "[-] NeoEngineSmokeTest executable tidak ditemukan di root build/."
fi

echo -e "\n[3] RECENT MODIFIED BINARIES / OBJECTS (Top 10):"
find "$BUILD_DIR" -type f \( -name "*.o" -o -name "*.a" -o -name "*.so" \) -printf '%T@ %p (%s bytes)\n' 2>/dev/null | sort -nr | head -n 10 | cut -d' ' -f2-

echo "=================================================="
