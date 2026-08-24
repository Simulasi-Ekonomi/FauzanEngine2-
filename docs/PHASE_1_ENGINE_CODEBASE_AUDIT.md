# Phase 1 — Audit Codebase dan Kontrak Kemampuan Engine

## Tujuan dan batas audit

Dokumen ini memulai fase pertama program FauzanEngine: membedakan source yang benar-benar aktif/teruji dari source legacy, editor/backend pendukung, dan placeholder. Audit ini tidak menghapus source historis, tidak mengaktifkan API eksternal, dan tidak mengubah kemampuan runtime.

## Inventaris filesystem reviewable

| Area | File source reviewable | Peran yang teridentifikasi | Status awal |
|---|---:|---|---|
| `Source/NeoEngine` | 825 | Canonical C++ runtime, rendering, physics, asset, scene, agent, template, dan platform contracts. | Referensi utama untuk klaim engine. |
| `Tests` | 134 | Smoke C++ per subsystem. | Bukti terbatas, bukan bukti release. |
| `engine` | 583 | Legacy/parallel engine dan tool historis. | Tidak boleh otomatis dianggap runtime aktif. |
| `editor` | 32 | Editor React/Three.js dan authoring UI. | Source tersedia; workflow game lengkap belum terbukti. |
| `backend` | 39 | API, scene authoring, dan agent legacy. | Perlu direkonsiliasi terhadap authority runtime. |
| `android` + `jni` | 11 | Gradle, activity, JNI/CMake, LiteRT manager. | Source tersedia; artifact/device evidence belum ada. |
| `skills` + `tools` | 35 | Tooling dan skill pendukung. | Tidak otomatis menjadi capability runtime game. |

Konfigurasi canonical `Source/NeoEngine/CMakeLists.txt` mendeklarasikan **92** entry source C++ runtime, atau **91** path unik setelah deduplikasi, serta **97** smoke target conditional. Empat smoke agent canonical sudah ada: autonomy policy, gateway, manual repair protocol, dan prompt-tool graph. Evidence broad non-Vulkan terakhir yang tercatat adalah 87/87 Release dan 87/87 ASAN; perbedaan antara target conditional dan executable yang dijalankan harus tetap dilacak, bukan disamakan secara otomatis.

Revalidasi pada awal fase ini menjalankan kembali seluruh executable smoke non-Vulkan yang tersedia: **87/87 Release** dan **87/87 AddressSanitizer** dengan `detect_leaks=1` lulus. Inventaris test berada terutama pada Runtime (50), Systems (12), Rendering (9), Farm (8), Animation (6), Templates (5), dan Agents (4). Banyak direktori source tidak memiliki direktori test bernama sama; ketiadaan nama test yang paralel bukan bukti tidak ada test, tetapi merupakan sinyal audit untuk domain Core, ECS, Editor, Networking, Platform, RHI, Scene, Streaming, Threading, dan World.

## Kontrak agent yang sudah benar-benar ada

`AgentCommandGateway` dan `PromptToolGraph` merupakan fondasi yang harus dipertahankan. Keduanya membatasi agent pada plan typed, dry-run, approval, build/test evidence, receipt, identifier aman, batas node/dependency, dan topological ordering. Gateway secara eksplisit menolak mutasi runtime dan ekonomi oleh agent.

Sebaliknya, `backend/aries/agents/aries_director.py` hanya merupakan orchestrator legacy LLM-to-JSON dengan endpoint eksternal tetap, token environment `GITHUB_TOKEN`, TLS verification dinonaktifkan, parsing JSON rapuh, task queue fixed, dan tanpa integrasi ke tool authority canonical. `OpenCodeIntegration.cpp` juga placeholder: URL localhost hardcoded, fallback menghasilkan fungsi TODO, response tidak divalidasi sebagai tool plan, dan tidak ada secret/provider abstraction. Keduanya tidak boleh dianggap AI engine produksi.

## Klasifikasi gap fase 1

| Kelompok | Bukti saat ini | Gap yang harus ditutup sebelum klaim capability |
|---|---|---|
| 2D/3D runtime | CPU mesh/sprite/scene proof dan sejumlah adapter tersedia. | Satu player-facing frame loop dengan input, surface presentation, asset-to-render binding, dan test scene end-to-end. |
| Asset/content | Registry, snapshot, staging, dan beberapa importer terbatas tersedia. | Schema proyek/prefab/gameplay terpadu, importer supported types, renderer-ready lifecycle, asset error/migration evidence. |
| Gameplay | Farm/RPG/route/physics/animation subsystems memiliki smoke terpisah. | Runtime ownership yang menyatukan karakter, building, item, NPC, monster, quest, navigation, collision, combat, dan save game. |
| AI game builder | Prompt graph/gateway safe; Aries/Hermes source ada. | Dialog/clarification, provider-neutral LLM boundary, structured schema, tool registry, approval queue, test fixtures, receipts, failure/rate-limit handling. |
| Editor/backend | Editor dan API authoring source ada. | Editor-to-engine import/export/build contract, durable project state, authenticated owner boundary, non-simulated packaging controls. |
| Android | Gradle/JNI source dan preflight tersedia. | Toolchain/device proof, reproducible debug APK, signed AAB, lifecycle/render/input/audio, policy/release evidence. |
| Online/release | Local contracts dan sandbox data tersedia. | Authoritative server, auth/session, durable persistence/recovery, anti-cheat, economy receipts, privacy/security, live operations, support/rollback. |

## Placeholder dan legacy treatment

Literal marker scans menemukan 17 file bertanda placeholder pada AI, ECS/editor legacy, RHI/FrameGraph, streaming, runtime capability, dan sistem item. Tidak satu pun file `.cpp` bertanda itu berada dalam 91 path unik CMake canonical saat audit ini; marker tersebut adalah source legacy/parallel atau header yang belum membentuk evidence runtime aktif. Editor Build/Publish/AI UI dan Android bridge juga memerlukan audit perilaku terpisah. Marker bukan alasan untuk menghapus file secara massal: setiap file harus diputuskan sebagai **retain**, **adapt**, **replace**, atau **exclude** berdasarkan caller nyata, build, dan acceptance evidence.

Perbandingan filesystem menemukan 782 file C++/header canonical dan 551 file pada mirror `engine/Source/NeoEngine`. Ada 546 path bersama: 339 identik dan 207 berbeda. Canonical juga memiliki 236 path eksklusif, sedangkan legacy memiliki lima path eksklusif. Karena lebih dari sepertiga path bersama berbeda, source legacy tidak boleh disalin atau ditimpa ke tree canonical secara otomatis. Setiap perbedaan memerlukan keputusan per modul, test target, dan migrasi eksplisit.

| Root source | Keputusan fase 1 | Aturan perubahan berikutnya |
|---|---|---|
| `Source/NeoEngine` + `Tests` | **Retain canonical** | Perubahan hanya melalui CMake, smoke, Release, dan ASAN evidence. |
| `engine/` | **Retain as legacy archive** | Adaptasi per modul; dilarang bulk copy ke canonical. |
| `editor/` | **Adapt** | Gunakan sebagai authoring client; klaim build/publish hanya setelah backend/runtime/package evidence. |
| `backend/api` | **Adapt** | Pertahankan authoring API yang typed dan versioned; tidak mendapat authority game runtime. |
| `backend/aries` | **Replace execution boundary** | Pertahankan ide/planning yang berguna; pindahkan ke provider-neutral, structured, approval-gated contract sebelum dipakai. |
| `android/` + `jni/` | **Adapt** | Pinned build/device evidence diperlukan; source tidak sama dengan APK readiness. |
| `skills/` + `tools/` | **Retain as development tooling** | Tidak otomatis dibundel ke runtime atau APK. |
| vault/cache/build output | **Exclude** | Tidak boleh menjadi dependency public repository atau artifact game. |

## Kontrak acceptance per capability

| Capability | Minimum acceptance evidence | Status awal |
|---|---|---|
| Project/scene/content authoring | Versioned schema, validation, migration, editor/backend/runtime receipt, rollback smoke. | Parsial. |
| 2D gameplay | Input-to-frame loop, sprite/tile asset rendering, collision/audio/UI/save integration smoke. | Belum terpadu. |
| 3D gameplay | Mesh/material/light/skeleton asset path to visible presentation surface, camera/input/collision smoke. | Belum terpadu. |
| NPC/monster/item/building | Typed data, runtime lifecycle, deterministic behavior/path/combat/persistence smoke. | Terisolasi/parsial. |
| Prompt game builder | Conversation clarification, JSON schema, dry-run plan, approval, tool receipt, audit/rollback smoke. | Gateway/graph parsial; provider/executor belum ada. |
| Android | Pinned toolchain, native/Gradle debug APK, signed AAB, emulator/device lifecycle/input/render/audio evidence. | Belum. |
| Post-release operations | Trusted server, session/auth, persistence/recovery, security/privacy, telemetry/rollback/appeal evidence. | Belum. |

## Keputusan arsitektur AI awal

Agent pembuat game akan mengikuti urutan berikut: user berdiskusi → assistant meminta klarifikasi → model menghasilkan output terstruktur → validator membangun dry-run `PromptToolPlan` → user menyetujui → executor allowlisted menjalankan authoring/build/test → receipt dan audit log dikembalikan. Tidak ada model atau tool yang boleh langsung mengubah runtime game live, ekonomi, ban, kredensial, atau deployment.

Provider LLM belum dipilih. Konektor OpenAI, Anthropic, dan Google Gemini terdeteksi dalam keadaan disabled. Pemilihan provider/API harus menunggu keputusan eksplisit mengenai provider, biaya, privasi, dan credential ownership. Source engine tidak akan mengandung key atau endpoint token hardcoded.

## Exit criteria fase 1

Fase 1 selesai hanya jika setiap domain pada TODO Phase 1.1–1.3 memiliki owner canonical, status retain/adapt/replace/exclude, acceptance test/gap reference, serta roadmap yang tidak mengklaim readiness sebelum evidence tersedia.
