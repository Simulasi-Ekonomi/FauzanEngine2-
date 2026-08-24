# Android APK/AAB Build Status

The Android source tree exists, but the sandbox currently has no Android SDK, NDK, `sdkmanager`, `adb`, or installed Gradle distribution. Its checked-in `local.properties` points to a Termux path that is unavailable in this environment. Therefore no APK or AAB has been produced, and the project must not be described as Play Store ready.

The static manifest blockers repaired in this pass are the missing `tools` XML namespace, the incorrect short activity name, and a telemetry service declaration without a corresponding Android class.

`scripts/preflight.sh` is the required fail-closed entry point before build. It verifies the SDK, Java 17, Gradle wrapper JAR, Android CMake 3.22.1, NDK, JNI CMake file, and—only for `release`—the four signing environment variables. Release Gradle configuration likewise refuses release/bundle tasks without signing values. No signing material is committed to source.

The native Android CMake file still targets a broad legacy `engine/Source/NeoEngine` source glob rather than the lean canonical runtime currently built on Linux. That wiring must be replaced with an Android-capable subset of the canonical CMake runtime and compiled on a host with Android SDK/NDK before a debug APK claim is valid. A Play Store AAB additionally requires a user-owned upload keystore, package identity review, policy-compliant permissions, store listing assets, and a successful signed bundle.

## Latest sandbox preflight

The fail-closed preflight ran successfully as a checker and correctly blocked both modes. Debug reported four blockers: unavailable/stale Android SDK, Java 17 not present, Android CMake 3.22.1 absent, and Android NDK absent. Release reported the same four blockers plus all four required signing environment variables absent. The Gradle wrapper JAR and JNI CMake file were found, but that alone is not sufficient to build.
