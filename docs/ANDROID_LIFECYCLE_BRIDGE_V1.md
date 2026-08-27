# Android Lifecycle Bridge V1

## Scope

`NeoEngineActivity` now invokes the existing canonical native lifecycle gate in the Android application lifecycle. It initializes the gate during `onCreate`, resumes only after a successful native initialization, pauses only while it has an active native state, and attempts shutdown during `onDestroy`. The Java bridge catches missing-native-library and missing-symbol errors and reports `false`; it does not retry through an untracked legacy native entry point.

The old activity button invoking a world-streaming native method was removed because that method is not provided by the audited canonical Android subset. The corresponding legacy Java calls now return `false` with an explicit unavailable message. The existing LiteRT path remains fail-closed as `LITERT_UNAVAILABLE`; the activity no longer copies an untrusted model file from external shared storage or calls the disabled initializer.

## Evidence

The source smoke verifies the Java-to-JNI symbol pairing and the create/resume/pause/destroy call sites, while rejecting retained direct activity routes to world streaming or LiteRT initialization. Native `android_lifecycle_gate_smoke` remains the Release and AddressSanitizer `detect_leaks=1` proof for gate transitions. The Android Gradle debug build compiles the Java bridge and canonical arm64 native subset into the debug package.

## Boundary

This is an application lifecycle wiring increment only. It is not evidence of Android device execution, render-surface handling, input/audio recovery, frame scheduling, APK install, release signing, AAB generation, crash/ANR behavior, Play policy review, local AI inference, world streaming, or production readiness.
