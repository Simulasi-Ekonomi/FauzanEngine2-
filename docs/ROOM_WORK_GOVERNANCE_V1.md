# FauzanEngine2 Multi-Room Branch Work Governance v1

**Status:** Binding engineering policy  
**Scope:** seluruh room/agent yang bekerja pada enam branch kerja:

- `gap-closure-all-branches-night`
- `p1-renderer-asset-animation-night`
- `p2-physics-networking-night`
- `p3-editor-android-production-night`
- `p3-pr44-advanced-port`
- `p4-release-certification-night`

**Canonical baseline:** branch `main` pada SHA yang sedang ditetapkan oleh integration owner.  
**Authoritative source:** `Source/NeoEngine/`.  
**Authoritative tests:** `Tests/` dan target yang terdaftar pada `Source/NeoEngine/CMakeLists.txt`.

## 1. Aturan paling penting

1. **Baca sebelum mengubah.** Setiap agent wajib membaca `AGENTS.md`, dokumen ini, `docs/AI_ENGINE_WORK_STANDARD.md`, status branch aktif, `todo.md`, CMake canonical, dan seluruh handover/ledger yang relevan terhadap scope.
2. **Satu scope, satu owner.** Sebelum coding, agent wajib menyatakan branch, base SHA, subsystem, file/path, gap ID, dan target smoke. Scope yang sudah dimiliki room lain tidak boleh dikerjakan ulang.
3. **Main adalah sumber kebenaran.** Branch lama, PR tertutup, dan laporan historis adalah evidence/rujukan; bukan alasan untuk menimpa `main` atau mengganti implementasi yang sudah canonical.
4. **Tidak ada merge massal.** Branch tidak boleh digabung dengan `main` atau branch lain hanya karena jumlah commit besar, file banyak, atau CI sebagian hijau. Integrasi dilakukan satu workstream per satu workstream.
5. **Tidak ada klaim selesai dari source saja.** Implementasi hanya berstatus `IMPLEMENTED-UNVERIFIED` sampai source terdaftar di CMake, dipakai pada runtime path yang benar, dan memiliki evidence executable.
6. **C1/CI dependency failure boleh dicatat terpisah, tetapi tidak boleh dipalsukan sebagai pass.** Jika C1 diabaikan oleh keputusan owner, setiap compile/runtime/test failure lain tetap blocker.

## 2. Registry wajib sebelum coding

Setiap room harus meninggalkan entri handover dengan format berikut sebelum mengedit:

```text
ROOM:
BRANCH:
BASE_SHA:
OWNER:
SCOPE:
GAP_IDS:
FILES_EXPECTED_TO_CHANGE:
CANONICAL_RUNTIME_PATH:
REQUIRED_CMAKE_TARGETS:
RELEASE_TARGETS:
ASAN_TARGETS:
BENCHMARK_OR_DEVICE_EVIDENCE:
KNOWN_OVERLAP_WITH_OTHER_BRANCHES:
```

Jika dua room mencantumkan file, symbol, target, atau Gap ID yang sama, pekerjaan kedua **berhenti** sampai scope disepakati. Jangan menyelesaikan overlap dengan copy-paste atau force overwrite.

## 3. Klasifikasi status capability

Setiap gap dan feature hanya boleh memiliki salah satu status berikut:

| Status | Arti |
|---|---|
| `OPEN` | Belum ada implementasi yang dapat dipakai |
| `IMPLEMENTED-UNVERIFIED` | Source/integrasi sebagian ada, tetapi evidence wajib belum lengkap |
| `INTEGRATED` | Source masuk canonical CMake dan live runtime path, smoke Release + ASAN lulus |
| `BENCHMARKED` | `INTEGRATED` plus workload/performance evidence yang dipersyaratkan |
| `DEVICE-VERIFIED` | Evidence target device/emulator/platform lulus bila diwajibkan |
| `CERTIFIED` | Seluruh gate release yang berlaku dan handover reproducible lulus |
| `BLOCKED` | Ada dependency, konflik, compile defect, atau evidence yang belum tersedia |

Dokumentasi, jumlah commit, jumlah baris, jumlah file, atau CI lint **tidak boleh** menaikkan status capability sendirian.

## 4. Pembagian scope enam branch

Pembagian ini mencegah pengulangan. Bila suatu perubahan melintasi scope, agent harus mencatat dependency dan meminta integration owner memindahkan atau memecah pekerjaan.

| Branch | Scope utama | Tidak boleh mengklaim |
|---|---|---|
| `gap-closure-all-branches-night` | runtime lifecycle, clock, Farm render invariants, canonical scope, dependency diagnostics | menutup P1–P4 hanya dengan workflow atau guard |
| `p1-renderer-asset-animation-night` | renderer, GPU skinning, animation pose/palette, asset streaming/render boundary | production Android, multiplayer, release certification |
| `p2-physics-networking-night` | XPBD measurement, replication, snapshot/ack/checksum, networking contracts | target 100K/<5 ms tanpa benchmark exact workload |
| `p3-editor-android-production-night` | editor contracts, scene authoring, audio, Android/JNI lifecycle, telemetry | Play Store readiness tanpa signing/device/policy evidence |
| `p3-pr44-advanced-port` | advanced editor/audio/renderer integration only where explicitly assigned | mengganti workflow atau menghapus gate branch lain tanpa keputusan owner |
| `p4-release-certification-night` | SBOM, provenance, artifact/reproducibility, release gates | menyatakan engine production-ready ketika P0–P3 belum certified |

## 5. Protokol anti-duplikasi

Sebelum menulis fitur baru, agent wajib:

1. `git grep` symbol/class/function yang akan dibuat pada `main` dan keenam branch.
2. Membaca implementasi terdekat, bukan hanya nama file.
3. Memeriksa CMake apakah source/target sudah terdaftar.
4. Memeriksa `todo.md`, branch status, ledger, dan smoke target terkait.
5. Jika ada implementasi serupa, pilih satu tindakan: **reuse**, **extend**, **repair**, atau **explicitly replace**. Alasan harus ditulis.
6. Tidak membuat `V2`, `Ultimate`, `Final`, `Bridge2`, atau duplicate subsystem hanya karena path lama sulit dipahami.
7. Tidak meng-copy source dari branch lain sebelum membandingkan diff dan ownership.

## 6. Kontrak test seragam

Semua room memakai canonical CMake berikut, kecuali task secara eksplisit membutuhkan workflow berbeda:

```bash
cmake -S Source/NeoEngine -B build/<scope>-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/<scope>-release --target <targets> -j2
./build/<scope>-release/<target>

cmake -S Source/NeoEngine -B build/<scope>-asan -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build/<scope>-asan --target <targets> -j2
ASAN_OPTIONS=detect_leaks=1 ./build/<scope>-asan/<target>
```

Aturan test:

- Test operation tidak boleh hanya hidup di `assert`; Release build dengan `NDEBUG` harus tetap menjalankan operasi.
- Release dan ASAN harus memakai **target source, dependency family, environment, dan input yang sama**.
- Smoke baru wajib terdaftar di CMake canonical dan masuk manifest/ledger.
- Test harus menguji success path, rejection/failure path, bounds, atomicity, ownership/lifetime, dan determinism yang relevan.
- Untuk async GPU/resource streaming, test wajib membuktikan submission tidak sama dengan completion: authoritative fence/timeline completion harus terjadi sebelum Ready/publication, serta release/eviction tidak boleh menghancurkan borrowed synchronization handles.
- Resource upload yang sedang in-flight wajib memiliki pin/lifetime contract di resource manager. `Release`, eviction, dan hot-reload harus menolak invalidasi resource selama pin aktif; pin harus dilepas tepat sekali pada successful completion atau cancellation/failure.
- Bila upload task menyimpan observer/pointer ke resource manager, resource manager wajib hidup lebih lama dari seluruh pending upload; uploader harus di-flush/dihancurkan sebelum owner resource manager.
- Benchmark wajib mencatat workload exact, build mode, hardware/runtime, elapsed time, dan exit status. Nama `bench_100k` bukan bukti 100K lulus.
- Device/Termux/Vulkan evidence tidak boleh digantikan oleh source inspection atau software fallback.

## 7. Definition of done

Satu gap baru boleh ditandai selesai hanya jika semua item yang berlaku terpenuhi:

- [ ] Gap ID dan acceptance criteria jelas.
- [ ] Implementasi berada di canonical source.
- [ ] CMake source/target registration ada.
- [ ] Runtime ownership/integration path terbukti.
- [ ] Rejection/failure behavior fail-closed.
- [ ] Release build dan smoke lulus.
- [ ] ASAN `detect_leaks=1` build dan smoke lulus.
- [ ] Benchmark/device/platform evidence tersedia bila dipersyaratkan.
- [ ] Tidak ada duplicate implementation atau unclassified marker baru.
- [ ] Handover diperbarui dengan SHA, file, command, result, dan remaining gaps.

Jika salah satu item belum ada, status tetap `IMPLEMENTED-UNVERIFIED` atau `BLOCKED`.

## 8. Zero-conflict merge protocol

1. Feature room tidak melakukan merge antar-feature branch.
2. Integration owner membuat integration branch dari `main` terbaru.
3. Branch di-rebase oleh owner/integration room, bukan oleh semua agent secara bersamaan.
4. Sebelum merge, owner wajib menjalankan:

```bash
git diff --check
 git merge-tree <base> <current-main> <feature-tip>
```

5. Konflik `CMakeLists.txt`, workflow, runtime ownership, public headers, atau manifest harus diselesaikan manual dan dicatat.
6. Delete/modify conflict tidak boleh diselesaikan otomatis dengan `-X theirs` atau `-X ours`.
7. Workflow yang dihapus harus memiliki alasan tertulis; tidak boleh menghapus gate hanya agar pipeline hijau.
8. Setelah setiap merge: configure, build, Release smoke, ASAN smoke, dan canonical scope audit diulang pada **tip gabungan yang sama**.
9. Push hanya non-force. Jika remote berubah, berhenti dan fetch/review ulang.

## 9. Handover wajib per room

Setiap room yang berhenti atau selesai wajib menulis:

```text
HANDOVER_SHA:
BRANCH:
SCOPE_COMPLETED:
FILES_CHANGED:
CMAKE_TARGETS_ADDED_OR_CHANGED:
RELEASE_COMMAND_AND_RESULT:
ASAN_COMMAND_AND_RESULT:
BENCHMARK_DEVICE_RESULT:
KNOWN_FAILURES:
UNVERIFIED_ITEMS:
NEXT_OWNER_AND_NEXT_ACTION:
```

Kalimat “done”, “fixed”, “ready”, atau “100%” tanpa command dan result dianggap tidak valid.

## 9.1 Runtime asset-stream handoff

Asynchronous `StreamManager` callbacks are handoff-only: they may enqueue immutable load results but must not directly mutate `AssetRegistry`, `AssetResourceManager`, or `AssetStreamingQueue`. Canonical state transitions occur on the runtime thread through the bridge pump; GPU Ready publication remains authoritative to fence/timeline completion.

## 10. Forbidden actions

- Force push, reset remote, delete branch, atau overwrite branch lain tanpa instruksi eksplisit owner.
- Menganggap command recording/submission sebagai GPU completion.
- Menghancurkan borrowed Vulkan fence/semaphore atau mendaftarkan owned synchronization handle lebih dari sekali.
- Menjadikan `gpuResident`/Ready hanya karena command recording atau queue submission berhasil.
- Membiarkan pending uploader task memegang `AssetResourceManager` yang sudah dihancurkan atau di-reset.
- Melepas GPU upload pin lebih dari sekali atau mengubah residensi menjadi Ready sebelum authoritative completion.
- Menghapus test/workflow untuk menyembunyikan failure.
- Mengurangi workload benchmark atau melonggarkan threshold.
- Mengubah ASAN/Release gate menjadi no-op.
- Menambah fallback palsu yang mengembalikan sukses.
- Mengklaim Unreal parity, production, APK, Play Store, multiplayer, anti-cheat, atau monetization readiness tanpa evidence yang sesuai.
- Mengerjakan ulang feature yang sudah `INTEGRATED`, kecuali task adalah regression repair.
- Menggabungkan branch hanya berdasarkan line count, commit count, atau label “turn 2”.

## 11. Aturan keputusan akhir

> **Zero gap** berarti seluruh gap yang berada dalam scope memiliki status dan evidence yang dapat diaudit; bukan berarti repository tidak memiliki pekerjaan masa depan.
>
> **Zero conflict** berarti integration tip tidak memiliki unresolved conflict, duplicate ownership, CMake ambiguity, atau workflow deletion yang tidak terdokumentasi.
>
> **100% capability** berarti implementasi, canonical integration, Release, ASAN, benchmark/platform evidence, failure-path behavior, dan handover semuanya tersedia pada revision yang sama.
>
> **Async GPU completion** berarti submission, authoritative completion, resource publication, renderer refresh, and release/eviction are separate auditable stages. A successful submission alone is never a Ready signal.

Dokumen ini adalah aturan lintas-room. Jika dokumen branch lama bertentangan dengan dokumen ini, aturan ini dan `docs/AI_ENGINE_WORK_STANDARD.md` berlaku; perbedaan harus dicatat, bukan diselesaikan dengan asumsi.
