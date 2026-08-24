# FauzanEngine Unity/Godot-Style Readiness Matrix — 23 Agustus 2026

## Cara membaca status

**Aktif terbukti** berarti code tercantum di CMake kanonis dan memiliki smoke executable yang lulus. **Parsial** berarti terdapat fondasi aktif tetapi belum memenuhi kontrak engine/game production. **Belum** berarti source tidak aktif, hanya legacy/scaffold, atau belum mempunyai capability/evidence yang diperlukan. Matriks ini sengaja tidak memakai status “ready” untuk sekadar header, kelas legacy, atau target yang belum dibuktikan.

| Domain checklist pengguna | Status | Evidence kanonis saat ini | Gap penting sebelum dianggap setara engine game nyata |
|---|---|---|---|
| Game loop, fixed tick, lifecycle | Parsial | `NeoRuntime`, `EngineLoop`, fixed tick Farm, init/tick/shutdown smoke. | Tidak ada Update/LateUpdate public, pause/timeScale, focus/resume platform, crash lifecycle, atau profiling frame. |
| Time system | Belum | Tick counter lokal pada world/authoring. | Tidak ada `deltaTime`, scale, pause, timer scheduler, maupun clock abstraction. |
| Logging/assert/debug file | Parsial | Assertion/compiler diagnostics dan smoke output. | Tidak ada structured logger, level, file rotation, crash report, console, atau debug UI. |
| Memory dan job system | Parsial | Pool/frame allocator dan JobSystem berada pada CMake. | Belum ada ownership contract lintas game, telemetry, cancellation, load profile, maupun tool debugging. |
| Project configuration | Parsial | Runtime/world/authority config bounded. | Tidak ada project setting serialized, platform override, secret separation UI, atau config migration. |
| Entity/component/scene | Parsial | ECS manager, `SceneWorld`, entity create/destroy/transform, Farm/authoring scene binding. | Tidak ada generic component API dengan scene load/unload/additive, prefab, tags/layers/find, pooling, atau editor. |
| Transform hierarchy | Parsial | Scene entity transforms dan parent relationship basic. | Local/world propagation, dirty optimization, constraints, editor hierarchy, and deterministic transform graph test masih belum lengkap. |
| Input | Parsial | SDL keyboard bridge/InputState smoke. | Mouse, touch, gamepad, action map, rebinding, virtual joystick, IME, accessibility, Android lifecycle belum ada. |
| Rendering surface/device | Parsial | Vulkan offscreen/texture/present proof dan SDL hidden surface. | Tidak ada runtime game presentation, swapchain recovery, camera, viewport, material authoring, or device support matrix. |
| 2D/3D renderer | Belum | CPU Farm overlay dan textured fullscreen Vulkan proof saja. | Sprite, mesh, camera, atlas, sort/order, batch, lighting, particles, TTF text, post-process, multi-camera semuanya belum ada. |
| UI | Belum | Tidak ada UI canvas/runtime. | Canvas, rect transform, controls, events, layout, safe area, theme, tween, accessibility belum ada. |
| Physics | Parsial | XPBD regression/determinism dan authored material contact contract. | Collider shapes, trigger, raycast, collision matrix, 2D API, production 3D API, serta material-to-XPBD wiring belum ada. |
| Animation | Belum | Skeleton definition bounded saja. | Sprite animation, skeletal pose/skinning, animator, state machine, blend tree, tween, timeline/cutscene belum ada. |
| Audio | Parsial | PCM AudioMixer dan SDL device/dummy smoke. | Asset decoder, BGM/SFX groups, fade, pitch, spatial audio, mobile focus/device loss, accessibility belum ada. |
| Assets | Parsial | Hash registry, PPM decode/staging, Vulkan transfer/sample proof. | General import, async loader, pak/bundle, refcount unload, addressable, hot reload, mesh/material/audio pipeline belum ada. |
| Save/serialization | Parsial | Farm/versioned authority checkpoint and authoring/world binary serialization. | Generic save manager, settings profile, persistent path, encrypted durable/cloud save, migration, backup/restore belum ada. |
| Scripting/logic/events | Parsial | C++ templates, bounded agent plan graph. | Script runtime, coroutine API, signal bus, data resource layer, sandboxed mod/script policy belum ada. |
| Tools/editor | Belum | Build/smoke tools only. | Scene editor, hierarchy, inspector, asset browser, play mode, console/debugger, prefab editing belum ada. |
| Desktop/mobile/web platform | Parsial | Linux test surface; Android toolchain preflight. | Windows/macOS/iOS/Web build, Android canonical bridge/device test/APK/AAB, resolution/orientation, packaging evidence belum ada. |
| Utilities | Parsial | Grid navigation, deterministic world generation, math/XPBD primitives, telemetry outbox. | A* or dynamic pathfinding, object pool contract, localization, debug draw, analytics governance, timer service belum ada. |
| Farm/Tower/Sudoku/Puzzle templates | Parsial | Bounded deterministic templates/smokes and Farm authority/world proof. | Complete playable presentation, content authoring, player input/UI, save, economy backend, online test, packaging, operations belum ada. |
| Prompt-executing AI | Parsial | DOCX/PDF/RAR bounded intake, model-selected prompt planner, typed graph, manual-repair proposal protocol. | No model invocation proof, operation executor, isolated workspace, build/test receipt, diff review, rollback worker, or approved source mutation. |

## Prioritas nyata untuk game ringan/menengah

Untuk Farm, Tower Defense, Sudoku, Puzzle, dan Idle, urutan implementasi yang paling bernilai bukan membangun editor atau aplikasi lebih dahulu. Fondasi berikut harus bertambah dengan bukti runtime: time/pause; action input dan touch; 2D camera/sprite/text/sorting; deterministic save/settings; data-driven definitions; 2D collision/raycast; tween/animation; object pooling; event/signal; UI safe-area; kemudian Android device vertical slice. Backend authority, persistence durable, dan anti-abuse tetap berjalan paralel sebelum multiplayer atau monetisasi dilakukan.

## Kesimpulan

FauzanEngine sekarang adalah **fondasi C++ backend/runtime yang berkembang dengan beberapa proof rendering, authority, world, navigation, asset, audio, dan authoring**. Ia belum mendekati parity Unity/Godot, belum dapat disebut engine game lengkap, dan belum dapat secara jujur membuat atau merilis game nyata tanpa pekerjaan besar pada renderer, UI, input, physics API, animation, assets, save, platform, toolchain, security, dan operasi.
