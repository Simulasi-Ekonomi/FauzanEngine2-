# Farm Vertical Slice Evidence V1

**Snapshot:** 27 Agustus 2026

Smoke kanonis `farm_vertical_slice_smoke` membuktikan satu alur player-facing yang dimiliki `NeoRuntime`, tanpa membuat loop `FarmRuntimeSession` kedua. Konfigurasi menggunakan Farm 4×4, HUD runtime, `SoftwareSurfacePresenter` hidden surface, dan binding input keyboard kanonis.

| Tahap | Evidence executable | Hasil |
|---|---|---|
| Input pemain | `InputState` movement binding → `FarmPlayerInputBridge` → `FarmWorldTool` | Player bergerak ke `(1,0)` dan receipt movement tercatat |
| Aksi/HUD | Routing keyboard `TabForward` + `Activate`, lalu interact pada tick berikutnya | `Till` terpilih tanpa mutasi langsung; tile `(1,0)` menjadi `Tilled` pada tick kanonis |
| Render/present | `NeoRuntime::RenderFarm` → software world/HUD composition → hidden software surface | Dua frame ter-render dan dua frame dipresent; world/HUD hash non-zero |
| Save/restore | `SaveFarmProgressCheckpoint(42)` dan `RestoreFarmProgressCheckpoint` | World/time kembali identik, revision `42` dipulihkan, receipt lama diinvalidasi |
| Failure-closed recovery | Checkpoint checksum dirusak lalu di-restore | Restore ditolak dengan `CheckpointDecodeFailed` tanpa menerima payload korup |

## Reproducible commands

```sh
cmake -S Source/NeoEngine -B build/main-release -DCMAKE_BUILD_TYPE=Release
cmake --build build/main-release --target farm_vertical_slice_smoke -j2
./build/main-release/farm_vertical_slice_smoke

cmake -S Source/NeoEngine -B build/main-asan \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS='-fsanitize=address -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address'
cmake --build build/main-asan --target farm_vertical_slice_smoke -j2
ASAN_OPTIONS=detect_leaks=0 ./build/main-asan/farm_vertical_slice_smoke
```

Hasil yang teramati pada snapshot ini adalah `FARM_VERTICAL_SLICE_SMOKE_OK input=1 action=1 render=2 present=2 checkpoint=1 restore=1 corrupt_reject=1` pada Release dan ASAN. `detect_leaks=0` dipakai mengikuti batas Vulkan/SDL/Mesa yang telah terdokumentasi; smoke ini tidak menggunakan Vulkan textured present.

> **Batas evidence:** ini adalah local, single-process, deterministic CPU/software vertical slice. Evidence ini tidak membuktikan audio output lifecycle, multiplayer, server authority durable, payment/entitlement provider, Android device behavior, GPU scene renderer, device-loss recovery, performance budget, atau production/release readiness. Mandatory release gates R1–R12 tetap memerlukan evidence terpisah.
