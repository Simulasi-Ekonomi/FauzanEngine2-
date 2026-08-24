#!/bin/bash
# FauzanEngine Android Build Tools

# Mencari lokasi gradlew secara otomatis di folder android
PROJECT_ROOT="/sdcard/Buku saya/FauzanEngine"
ANDROID_DIR="$PROJECT_ROOT/android"
BUILD_OUTPUT="$PROJECT_ROOT/Builds/Android"

mkdir -p "$BUILD_OUTPUT"

if [ ! -d "$ANDROID_DIR" ]; then
    echo "❌ Error: Folder 'android' tidak ditemukan di $PROJECT_ROOT"
    exit 1
fi

cd "$ANDROID_DIR"

case $1 in
    "clean")
        echo "🧹 Membersihkan cache build..."
        chmod +x gradlew
        ./gradlew clean
        ;;
    "debug")
        echo "🏗️ Memulai Build APK Debug di $ANDROID_DIR..."
        chmod +x gradlew
        ./gradlew assembleDebug
        if [ $? -eq 0 ]; then
            mkdir -p "$BUILD_OUTPUT"
            cp app/build/outputs/apk/debug/*.apk "$BUILD_OUTPUT/"
            echo "✅ Build Sukses! APK ada di: $BUILD_OUTPUT"
        else
            echo "❌ Build Gagal! Cek koneksi internet atau konfigurasi SDK."
        fi
        ;;
    *)
        echo "Usage: ./android_tools.sh {clean|debug}"
        ;;
esac
