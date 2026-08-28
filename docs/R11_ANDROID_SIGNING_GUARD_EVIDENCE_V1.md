# R11 Android Signing Guard Evidence V1

## Scope

`android/app/build.gradle` mengharuskan empat environment values (`NEO_ANDROID_KEYSTORE`, `NEO_ANDROID_KEY_ALIAS`, `NEO_ANDROID_STORE_PASSWORD`, dan `NEO_ANDROID_KEY_PASSWORD`) untuk task release/bundle. Tanpa seluruh values tersebut, Gradle melempar `GradleException` sebelum release APK/AAB dibangun. Debug packaging tidak dianggap sebagai release signing evidence.

## Evidence

```text
ANDROID_RELEASE_SIGNING_GUARD_SMOKE_OK required_keys=4 fail_closed=1 secrets_read=0
```

`tools/android_release_signing_guard_smoke.sh` memeriksa source contract, variable names, fail-closed exception, dan release signing assignment. Smoke tidak membaca nilai secret, tidak membuat key, dan tidak memasukkan credentials ke repository.

Existing Android evidence juga mencakup canonical native lifecycle/debug package path, tetapi increment ini tidak menjalankan perangkat Android.

## Boundary and status

Belum ada signed release AAB, release APK hash/provenance, emulator/device smoke, crash/ANR evidence, Play policy review, store assets, production keystore, atau owner approval. Karena itu **R11 tetap Not passed**. Missing signing secrets adalah kondisi fail-closed yang disengaja, bukan alasan untuk mengarang artifact atau secret.
