# R8 Persistence and Recovery Evidence V1

## Scope

Canonical runtime persistence sekarang memiliki dua lapisan evidence yang aktif di CMake. `RuntimeSettingsStore` dan `RuntimeSaveCodec` menggunakan envelope version, batas ukuran, checksum FNV-1a, schema/kind validation, trailing-byte rejection, serta sensitive-content rejection. `AtomicSaveFile` menulis slot melalui temporary file dan rename, lalu menyediakan `Backup` dan `RestoreBackup` yang memakai temporary backup file, validasi slot, dan batas payload yang sama.

## Release and ASAN evidence

Target berikut lulus pada konfigurasi Release dan AddressSanitizer dengan `ASAN_OPTIONS=detect_leaks=1`:

```text
ATOMIC_SAVE_FILE_SMOKE_OK write=1 read=1 codec=1 slotValidation=1 backup=1 restore=1 missingRestorePreserved=1
RUNTIME_PERSISTENCE_SMOKE_OK settings=2 atomic=1 save=1 checksum=1 sensitive=1
NEO_RUNTIME_CHECKPOINT_SMOKE_OK progress=atomic topology=preserved authority=rebound
FARM_COMMERCE_CHECKPOINT_FILE_SMOKE_OK file_restore=1 replay_rejected=1 fail_closed=1 atomic=1
```

`atomic_save_file_smoke` membuktikan round-trip codec, backup snapshot, overwrite, restore ke payload sebelumnya, invalid slot, missing slot, dan bahwa kegagalan restore dari backup yang hilang tidak mengubah payload slot utama. Smoke runtime membuktikan decode corruption/checksum/trailing/sensitive-content failure mempertahankan state caller yang sudah valid. Checkpoint smokes membuktikan restore atomik serta penolakan replay/corrupt payload.

## Boundary and status

Increment ini adalah **local bounded file evidence**, bukan bukti persistence production. Belum ada authoritative database, multi-process locking, fsync/power-loss proof, encrypted-at-rest storage, retention/deletion policy, privacy access control, schema migration service across deployed versions, backup rotation, disaster-recovery drill, cloud restore, or operational RPO/RTO. Karena itu **R8 tetap Not passed** dalam release-readiness matrix.
