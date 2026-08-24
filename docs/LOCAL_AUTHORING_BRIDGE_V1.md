# Bridge Authoring Lokal v1

Bridge ini adalah adapter **dalam-proses dan approval-gated**. Ia tidak membuka socket, endpoint HTTP, command shell, akses file arbitrer, maupun otoritas pemain. Pemanggil lokal terlebih dahulu menyetujui request, lalu menyerahkan byte handoff yang dibatasi ke runtime C++.

## Envelope biner

| Field | Encoding | Batas |
|---|---|---|
| magic | empat byte ASCII `NAB1` | Wajib persis. |
| version | `u8` | Wajib `1`. |
| scene ID | `u8` length + UTF-8 ASCII | 1–48 byte; sama dengan `SceneDocument v1`. |
| revision | little-endian `u64` | Minimal `1`. |
| actor count | little-endian `u16` | Maksimal `512`. |
| actor | ID `u32`, parent ID `u32`, kind `u8`, sembilan `f32`, asset-ID `u8` length + ASCII | Asset ID maksimal 128 byte. |

Actor memakai ID dan parent yang sama dengan `SceneDocument v1`; `parent ID` nol berarti root. Kind dipetakan satu banding satu pada enum `EditorSceneActorKind`. Semua number dan length diperiksa sebelum alokasi atau mutasi state.

## Approval dan receipt

Pemanggil harus menyerahkan `approved=true`. Jika tidak, bridge menolak request dengan `ApprovalRequired` sebelum parsing. Jika parsing, reference asset, hierarchy, atau transform gagal, bridge mempertahankan `SceneWorld` target sebelumnya dan mengembalikan error terstruktur.

| Receipt | Makna |
|---|---|
| `sceneId` | ID authoring yang lolos validasi. |
| `revision` | Revision backend yang dibawa handoff. |
| `actorCount` | Jumlah actor yang berhasil dimuat. |
| `payloadDigest` | FNV-1a 64-bit atas byte handoff untuk audit/debug lokal, bukan signature keamanan. |

> Envelope ini belum merupakan format jaringan atau save game. Backend JSON dan editor hanya boleh menghasilkan handoff ini melalui adapter lokal yang disetujui; tidak ada jalur publik yang dapat mengirimkannya langsung ke runtime game.

## Bukti implementasi

`LocalAuthoringBridge` C++ menolak payload tanpa approval, magic/version/length yang rusak, trailing byte, reference asset yang hilang, dan scene yang tidak lolos `EditorSceneDocumentAdapter`. Smoke bridge juga memverifikasi bahwa `SceneWorld` tetap utuh setelah penolakan. Serializer backend mengharuskan approval eksplisit dan memakai fixture payload identik; FNV-1a receipt yang diuji pada kedua lapisan adalah `5832217868230341815`. Evidence ini membuktikan kompatibilitas bounded bridge v1, bukan koneksi jaringan atau permission deployment.
