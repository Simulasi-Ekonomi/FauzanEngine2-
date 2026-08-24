# FauzanEngine Android dan Google Play Release Path

## Status saat ini

Toolchain sandbox telah tersedia: JDK 17, Android command-line tools, platform-tools, API 36, build-tools 36.0.0, NDK r29 `29.0.14206865`, dan CMake 3.22.1. Preflight awal mengonfirmasi toolchain dan menemukan path source legacy. CMake JNI kini memakai `Source/NeoEngine` kanonis dengan daftar source eksplisit; build cross-compile NDK menghasilkan `libneo_core.so` ELF `arm64-v8a` untuk subset runtime/persistence/economy yang diaudit. Build ini **bukan** Gradle packaging dan tidak menghasilkan APK/AAB. Tidak ada keystore, emulator/device test, Play Console app, atau rilis yang dibuat.

> **Toolchain atau library native tersedia bukan berarti game Android siap rilis.** APK/AAB tidak boleh dibangun hingga bridge native mencakup runtime yang dibutuhkan, gameplay/render/input Android terbukti pada device, dan release gate backend yang lebih dahulu masih terbuka telah dipenuhi.

## Urutan engineering

| Gate | Bukti wajib | Status |
|---|---|---|
| A1. Toolchain reproducible | Preflight mengunci SDK/NDK/CMake/JDK dan lisensi. | **Lulus di sandbox.** |
| A2. Canonical native bridge | CMake Android memakai `Source/NeoEngine` kanonis, daftar source eksplisit, ABI `arm64-v8a`, dan JNI/lifecycle yang diuji. | **Subset native lulus.** `libneo_core.so` arm64 untuk runtime/persistence/economy/lifecycle subset berhasil melalui CMake NDK; simbol `JNI_OnLoad`, profile, dan `nativeLifecycleEvent` diekspor. Host smoke membuktikan state gate fail-closed, tetapi JNI belum lifecycle Android activity atau gameplay lengkap. |
| A3. Mobile runtime vertical slice | Surface/window Android, Vulkan/renderer runtime, touch/back lifecycle, audio focus, pause/resume, storage sandbox, crash capture. | **Belum.** Proof SDL/Vulkan desktop bukan device Android. |
| A4. Debug artifact | Debug APK reproducible, native symbols, install/launch pada emulator dan perangkat fisik, smoke gameplay Farm. | **Belum dibuat.** |
| A5. Release artifact | Versioning, shrink/obfuscation review, SBOM/license scan, signed AAB, upload key dikelola aman di luar repo. | **Belum.** |
| A6. Play Console readiness | Store listing, content rating, privacy/data safety, policy declarations, country/pricing, support contact. | **Belum.** |
| A7. Testing dan launch | Internal/closed test, pre-launch reports, Android vitals, observability, rollback/halt procedure, staged rollout. | **Belum.** |

## Ketentuan platform saat ini

Mulai **31 Agustus 2026**, aplikasi baru dan update aplikasi Google Play harus menargetkan Android 16 / API 36 atau lebih tinggi; `compileSdk` dan `targetSdk` Android kini disetel ke API 36. NDK r29 pada sandbox melaporkan platform native tertinggi API 35 ketika diminta `latest`; hal ini tidak menghasilkan APK/AAB dan belum membuktikan device compatibility atau kepatuhan rilis.[1]

Google Play mendistribusikan artifact baru lewat Android App Bundle; bundle perlu ditandatangani upload key sebelum upload dan Play App Signing mengelola app-signing key. Keystore private serta password tidak boleh disimpan dalam repository atau context game.[2]

| Tahap Play Console | Bukti yang diperlukan sebelum maju | Status FauzanEngine |
|---|---|---|
| Create app | Developer account, package name permanen, game/app category, support email, policy dan export declarations. | Tidak dilakukan. |
| App signing | Upload key terpisah, storage secret aman, AAB release tervalidasi. | Tidak dilakukan. |
| Test tracks | Internal hingga 100 tester, kemudian closed/open sesuai kebutuhan; personal account baru dapat memiliki kewajiban test tambahan. | Tidak dilakukan. |
| Review dan production | Listing/setup/review lengkap, bundle diterima, pre-launch report, policy/privacy, controlled rollout. | Tidak dilakukan. |
| Operasi pascarilis | Android vitals, alerting, halt/rollback procedure, incident ownership dan support. | Tidak dilakukan. |

## Batas eksekusi

Jalur ini tidak memberi izin untuk menandatangani artifact, membuat developer account, mengisi listing, mengunggah bundle, atau memulai test track. Semua tindakan tersebut memerlukan artefak yang lolos gate A2–A5, kredensial pihak pengguna, serta konfirmasi eksplisit sebelum tindakan external yang sensitif.

## References

[1] [Meet Google Play's target API level requirement](https://developer.android.com/google/play/requirements/target-sdk)

[2] [Sign your app](https://developer.android.com/studio/publish/app-signing)

[3] [Create and set up your app — Play Console Help](https://support.google.com/googleplay/android-developer/answer/9859152?hl=en)

[4] [Prepare and roll out a release — Play Console Help](https://support.google.com/googleplay/android-developer/answer/9859348?hl=en)

[5] [Publishing overview — Play Console](https://play.google.com/console/about/publishingoverview/)
