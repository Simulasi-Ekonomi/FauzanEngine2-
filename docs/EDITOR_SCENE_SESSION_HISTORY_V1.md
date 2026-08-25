# EditorSceneSession History v1

## Tujuan

Fase B.5a menambahkan undo dan redo bounded untuk mutasi scene editor yang telah berhasil. History menyimpan snapshot `EditorSceneDocument` in-memory, bukan delta transform atau write langsung ke runtime. Setiap pemulihan snapshot tetap melewati `OpenCandidate`, sehingga document adapter serta mesh/sprite asset binding divalidasi kembali sebelum state session berubah.

> History v1 hanya menyimpan paling banyak 32 snapshot tiap arah. Ia bukan event-sourced journal, persistence log, multi-user history, atau sistem undo/redo produksi.

## Kontrak operasi

| Jalur | Kontrak |
|---|---|
| Mutasi berhasil | Snapshot document sebelum mutasi masuk ke undo history; redo history dikosongkan. |
| `Undo` berhasil | Snapshot undo teratas dibangun lewat candidate Open; document sebelumnya masuk ke redo history. |
| `Redo` berhasil | Snapshot redo teratas dibangun lewat candidate Open; document sebelumnya masuk ke undo history. |
| Mutasi branch baru setelah undo | Redo history dibuang setelah candidate mutasi sukses. |
| Candidate gagal | Session aktif dan kedua history tidak berubah; item tidak dipop sampai candidate berhasil. |
| Open/Revert eksplisit berhasil | Kedua history dibersihkan karena baseline session baru dipilih. |
| History kosong | Operasi ditolak dengan `HistoryUnavailable`. |

Semua history bersifat in-memory dan bounded. Bila vector history sudah mencapai 32 entry, snapshot tertua dari arah tersebut dilepas sebelum snapshot baru ditambahkan. Tidak ada write transform paralel atau perubahan terhadap RouteIntent, MovementAuthorityGate, physics, atau agent authority.

## Evidence executable

`editor_scene_session_history_smoke` membuktikan transform/reparent undo-redo, redo invalidation setelah branch `AddActor`, penolakan undo kandidat setelah texture asset dibuat tidak dapat dibind sambil mempertahankan session/history, serta penolakan undo/redo pada session kosong.

| Gate | Hasil final |
|---|---|
| `editor_scene_session_history_smoke` Release | Lulus; undo, redo, branch clear, dan failure atomic dibuktikan. |
| `editor_scene_session_history_smoke` AddressSanitizer | Lulus dengan `ASAN_OPTIONS=detect_leaks=1`. |
| Broad non-Vulkan Release | 116/116 smoke lulus. |
| Broad non-Vulkan AddressSanitizer | 116/116 smoke lulus dengan `detect_leaks=1`. |

## Batas terbuka

History tidak merekam input perintah, tidak mendukung grouped transactions, merge/branch UI, persistence/file save, crash recovery, collaboration/conflict resolution, asset hot reload, editor desktop UI, GPU runtime, APK/AAB, atau release readiness. Batas ini harus dipertahankan sampai tiap capability memiliki kontrak dan evidence sendiri.
