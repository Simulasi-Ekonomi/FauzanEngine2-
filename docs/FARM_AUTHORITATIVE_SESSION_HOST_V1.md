# Farm Authoritative Session Host V1

`FarmAuthoritativeSessionHost` adalah boundary proses-lokal yang menempatkan principal session terautentikasi di sisi host sebelum `FarmAuthoritativeService`. Caller hanya menyerahkan `FarmSessionCommand`; host membangun `AuthorityCommand` internal memakai `playerId` dan `sessionId` yang tersimpan dari `FarmSessionPrincipal`. Nama pemain yang diklaim command wajib identik dengan principal tersimpan; subject spoofing ditolak sebelum layanan Farm dipanggil.

Setiap autentikasi ulang untuk pemain yang sama merotasi handle session. Handle lama tidak lagi ditemukan, sehingga command lama ditolak. Host tidak menulis ekonomi atau ban secara langsung: seluruh command tetap melewati `FarmAuthoritativeService`, `AuthoritativeCommandGate`, dan `TrustSafetySystem`. Dengan demikian, urutan client, batas laju, replay command ID, validasi tick, handler Farm, dan pemblokiran ban tetap berada pada gate kanonis.

Command yang diterima menghasilkan receipt V1 berisi keputusan otoritatif, snapshot world berhash, dan delta dari snapshot sebelum ke snapshot sesudah. Snapshot baru adalah salinan milik receipt; perubahan buffer receipt oleh caller tidak mengubah `LastReceipt()` host. Replay idempoten diterima sebagai replay tanpa delta perubahan state.

> Ini adalah boundary in-memory dalam satu proses, bukan transport jaringan. Tidak ada TLS, token kriptografis, penyimpanan sesi durable, discovery/matchmaking, replikasi delta melalui socket, otorisasi lintas-proses, observability operasi, atau bukti multiplayer/produksi.

`farm_authoritative_session_host_smoke` membuktikan penolakan principal tidak valid, handle tidak dikenal/stale, spoofed subject, dan handler Farm yang ditolak; juga membuktikan command valid, replay idempoten, rotasi session, snapshot/delta, dan isolasi salinan receipt pada Release dan AddressSanitizer `detect_leaks=1`.
