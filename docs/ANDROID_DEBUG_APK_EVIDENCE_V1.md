# Android Debug APK Evidence V1

Increment P0.6a memulihkan jalur build APK debug dari modul kanonis `android/app`. Preflight berhasil dengan Android SDK platform 36, NDK `29.0.14206865`, CMake `3.22.1`, dan Java 17. `:app:assembleDebug` kemudian berhasil dengan arm64 native `libneo_core.so` di dalam artefak.

Artefak yang diperiksa adalah `app-debug.apk`, package `com.neoengine.editor`, `minSdk 26`, `targetSdk 36`, dan signature APK Scheme v2 bertanda **Android Debug**. Hash SHA-256 build yang diuji adalah `c4488985c3c69d326eefb50020c7799bfa2efe95af347af5c4c5fdec04d7e2e3`. Pemeriksaan daftar arsip menemukan `lib/arm64-v8a/libneo_core.so` dan manifest.

Ketergantungan LiteRT yang memakai `latest.release` dibatasi fail-closed dari artefak debug ini karena versi dependency yang terunduh membutuhkan metadata Kotlin lebih baru daripada compiler proyek. `NeoEngineBridge.initLiteRT` tidak mengeksekusi prompt dan `sendPrompt` secara eksplisit menghasilkan `LITERT_UNAVAILABLE`; tidak ada klaim model lokal atau agent Android yang berfungsi.

> Bukti ini hanya membuktikan paket debug dapat dikompilasi dan ditandatangani debug dalam sandbox. Tidak ada perangkat atau emulator yang tersambung ketika diperiksa, sehingga tidak ada bukti instalasi, launch, rendering surface, input, audio, native crash/ANR, model AI, signed release APK/AAB, Play Console, maupun Play Store readiness.
