# Network Boundary Evidence V1

**Snapshot:** 27 Agustus 2026

Commit network terbaru menambahkan primitive header-only yang kini memiliki target smoke di CMake canonical. Wiring ini sengaja membuktikan kontrak deterministik tanpa menganggapnya sebagai transport socket atau multiplayer production.

| Area | Target smoke | Evidence |
|---|---|---|
| Transport-neutral queue | `network_transport_smoke` | Enqueue/dequeue bounded, reliable/unreliable metadata, tail reorder, duplicate/drop fault injection, stats, dan radius interest filter |
| Session prediction | `network_session_smoke` | Client prediction, server ownership check, sequence duplicate rejection, input magnitude rejection, reconciliation, snapshot checksum/sequence acceptance |
| Replication policy | `network_replication_policy_smoke` | Interest selection bounded dan prioritization deterministik; null input ditolak |

Semua smoke di atas lulus pada Release pada snapshot ini. Test tidak lagi memakai `assert`, sehingga pemeriksaan tetap aktif ketika `NDEBUG` berlaku.

> **Batas evidence:** implementasi ini belum menyediakan socket I/O, TLS, serialization wire format, packet loss recovery, matchmaking, durable session, server deployment, Farm authority integration, adversarial load, atau client security. `P0.4` tetap **Not passed**; increment ini hanya menutup primitive bounded dan executable coverage yang terisolasi.

## Reproducible commands

```sh
cmake -S Source/NeoEngine -B build/main-release -DCMAKE_BUILD_TYPE=Release
cmake --build build/main-release \
  --target network_transport_smoke network_session_smoke network_replication_policy_smoke -j2
./build/main-release/network_transport_smoke
./build/main-release/network_session_smoke
./build/main-release/network_replication_policy_smoke
```

Expected output prefixes are `NETWORK_TRANSPORT_SMOKE_OK`, `NETWORK_SESSION_SMOKE_OK`, and `NETWORK_POLICY_SMOKE_OK`.
