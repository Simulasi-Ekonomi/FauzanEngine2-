# Android Toolchain Research — 23 Agustus 2026

Dokumen ini menyimpan sumber resmi yang dipakai sebelum provisioning Android sandbox. Pengguna telah mengonfirmasi penerimaan Android SDK License Agreement untuk environment ini.

| Keperluan | Temuan resmi | Sumber |
|---|---|---|
| Command-line tools Linux | Paket `commandlinetools-linux-15859902_latest.zip`, checksum SHA-256 `4e4c464f145a7512b57d088ac6c278c03c9eea610886b35a5e0804e74eedf583`. | [Android Studio download](https://developer.android.com/studio) |
| SDK package management | `sdkmanager` memasang/memperbarui package ke direktori SDK; `ANDROID_HOME` direkomendasikan untuk command line. | [Command-line tools](https://developer.android.com/tools) |
| Native build prerequisites | NDK, CMake, dan LLDB dibutuhkan untuk kompilasi/debug native; NDK dapat dipin dengan `android.ndkVersion`. | [Install and configure NDK and CMake](https://developer.android.com/studio/projects/install-ndk) |
| NDK current | NDK stable `r29`, `ndkVersion "29.0.14206865"`; LTS `r27d`, `27.3.13750724`. Pekerjaan ini memilih stable r29 untuk preflight toolchain, bukan sebagai bukti kompatibilitas app. | [NDK downloads](https://developer.android.com/ndk/downloads) |
| Signing/release | APK harus ditandatangani; Android App Bundle untuk Google Play harus ditandatangani upload key sebelum upload, dan Play App Signing mengelola app-signing key. | [Sign your app](https://developer.android.com/studio/publish/app-signing) |

Provisioning tidak berarti APK/AAB, device test, signing key, Play Console, atau publish sudah dilakukan. Keystores dan kredensial Play tidak boleh dibuat atau diletakkan di repository tanpa otorisasi eksplisit dan secure secret handling.
