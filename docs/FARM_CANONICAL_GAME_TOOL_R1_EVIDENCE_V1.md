# Farm Canonical Game Tool — R1 Evidence V1

**Snapshot:** 27 Agustus 2026

## Scope

`FarmCanonicalGameTool` adalah contract typed/versioned di atas jalur aktif `FarmWorldTool`. Contract ini memisahkan rules, content registry, command input, command receipts, save/load format, migration, dan deterministic state dari loop runtime Farm. Ia tidak mengaktifkan source legacy dan tidak membuat loop Farm kedua.

## Acceptance matrix

| R1 requirement | Canonical implementation | Executable evidence |
|---|---|---|
| Typed world contract | `FarmWorldTool` sebagai world binding; command contract memanggil `SetCharacterState`/`PlayerTill` | `farm_canonical_game_tool_smoke` menerima command typed dan menghasilkan receipt |
| Versioned rules | `FarmToolRules`, current payload version `2`, bounded field validation | Save/load mempertahankan rules; invalid rules ditolak tanpa state mutation |
| Versioned content | Sorted `FarmToolContentEntry` registry dengan ID, revision, dan asset hash | Duplicate/unsafe IDs ditolak; content diserialisasi dan dipulihkan deterministically |
| Save migration | Current v2 payload dan compatibility v1 payload; v1 mengangkat content/rules/world/sequence ke v2 | v1→v2 load menghasilkan deterministic state yang sama |
| Invalid input | Sequence monotonicity, command kind, movement bound, content path safety, checksum/truncation rejection | Invalid content/rules/duplicate sequence/over-distance/corrupt payload ditolak fail-closed |
| Deterministic replay | Prevalidated ordered command batch, atomic rollback on failed apply, deterministic state hash | Dua tool instance dengan world/rules/content/commands identik menghasilkan hash sama |

## Reproducible evidence

Release:

```text
FARM_CANONICAL_GAME_TOOL_SMOKE_OK version=2 content=2 commands=3 migration=1 invalid=1 deterministic=1 bytes=751 state=14945317586204746748
```

AddressSanitizer with leak detection enabled:

```text
FARM_CANONICAL_GAME_TOOL_SMOKE_OK version=2 content=2 commands=3 migration=1 invalid=1 deterministic=1 bytes=751 state=14945317586204746748
```

Target tersebut terdaftar pada `Source/NeoEngine/CMakeLists.txt` dan dibangun dari `FarmCanonicalGameTool.cpp`, `FarmWorldTool.cpp`, serta canonical runtime source set. Smoke juga memverifikasi bahwa corrupt load tidak mengubah state yang sudah valid.

GitHub Actions workflow `R1 Canonical Game Tool` run **33089796273** pada main SHA **`347475d`** lulus untuk konfigurasi Release dan AddressSanitizer. Workflow menginstal dependency native runner, mengonfigurasi CMake/Ninja, membangun target smoke, lalu menjalankan kedua konfigurasi tersebut.

## Status

R1 dinyatakan **Passed untuk canonical Farm tool scope** berdasarkan evidence di atas. Status ini tidak menyatakan seluruh release siap, karena R2–R12 tetap memiliki gate terpisah.

> **Template boundary:** matrix release mewajibkan setiap game template membuktikan R1–R12 secara independen. Evidence ini membuktikan Farm canonical tool contract; RPG atau template lain tidak boleh dianggap lulus hanya karena memakai shared engine code.

> **Production boundary:** evidence ini belum membuktikan production authoring UI lengkap, remote content service, multiplayer, durable backend persistence, provider commerce, signing, device matrix, atau release readiness.
