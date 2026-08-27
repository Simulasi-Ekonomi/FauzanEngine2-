# Farm Authority Reconnect Evidence V1

## Scope

`FarmAuthoritativeSessionLoopback` kini dapat meminta maksimum dua koneksi TCP localhost berurutan melalui `AuthorityLoopbackServer`. Reconnect bersifat opt-in; transport umum tetap default satu koneksi agar rejection/stop semantics yang sudah ada tidak berubah. Koneksi kedua menggunakan dispatcher dan `FarmAuthoritativeSessionHost` yang sama, termasuk server-authenticated principal dan session handle yang sama.

## Evidence

`farm_authoritative_session_loopback_smoke` membuktikan:

| Capability | Evidence |
|---|---|
| Initial authenticated command | One `farm.till` command is accepted and produces authoritative revision 1. |
| Same-connection replay | Replaying the same command returns the same revision/state without a second Farm mutation. |
| Disconnect/reconnect | The first TCP client closes, a second client connects to the same localhost port, and the same command is replayed through the same session host. |
| Snapshot stability | Initial, same-connection replay, and reconnect replay snapshots have identical revision/state. |
| Principal/session rejection | Subject-spoof and wrong-session commands remain rejected before Farm dispatch. |
| Compatibility | Raw `authority_loopback_transport_smoke` continues to pass with default one-connection behavior. |

Release and ASAN output:

```text
FARM_AUTHORITATIVE_SESSION_LOOPBACK_SMOKE_OK revision=1 replay=1 reconnect=1 subject_rejected=1 session_rejected=1
AUTHORITY_LOOPBACK_TRANSPORT_SMOKE_OK port=loopback revision=1 handled=1 idleStop=1
```

## Boundary

This is bounded localhost TCP reconnect evidence. It does not prove TLS, public networking, NAT traversal, durable session storage, token rotation, multi-client concurrency, snapshot/delta wire replication, load/capacity, deployment, or multiplayer production readiness. The reconnect window is deliberately capped at two sequential connections.
