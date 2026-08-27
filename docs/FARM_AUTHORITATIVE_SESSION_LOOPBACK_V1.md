# Farm Authoritative Session Loopback V1

## Scope

`FarmAuthoritativeSessionLoopback` composes the existing in-memory `FarmAuthoritativeSessionHost` with the existing `AuthorityLoopbackServer`. At start, the hosting process supplies a principal that it has already authenticated. The adapter obtains and retains only the opaque host session handle; each wire command must present exactly the bound player and session identifiers before the adapter converts it to the narrower `FarmSessionCommand` accepted by the host.

The adapter does not let a wire command select another Farm subject or replace its bound session. A player or session mismatch is rejected before `FarmAuthoritativeSessionHost::Submit`, so no Farm action, revision increment, or snapshot is committed. Rejected dispatches intentionally fail snapshot construction; the existing bounded loopback transport then closes the connection with `WireRejected` rather than returning Farm world bytes to a mismatched caller.

For an accepted first command or canonical replay, the adapter returns the copy-owned authoritative snapshot already attached to the host receipt. The snapshot builder requires matching nonzero revision and nonempty state; it does not synthesize or reuse a snapshot after a rejection.

## Executable Evidence

`farm_authoritative_session_loopback_smoke` uses real localhost TCP framing and proves the following in both Release and AddressSanitizer with `ASAN_OPTIONS=detect_leaks=1`:

| Case | Required result |
|---|---|
| Uninitialized host | Adapter refuses start without opening a listener. |
| Bound principal command | One `farm.till` advances authority revision to one and returns a deserializable snapshot. |
| Canonical replay | Same command returns the same revision and immutable world bytes. |
| Spoofed player | No response snapshot is delivered; transport closes with `WireRejected`; Farm state and revision remain unchanged. |
| Stale/wrong session | No response snapshot is delivered; transport closes with `WireRejected`; Farm state and revision remain unchanged. |

## Boundary

This is a **localhost-only, single-connection, bounded transport adapter**. It is not a TLS protocol, token verifier, durable session store, cross-process authorization service, matchmaking system, delta replication service, public network listener, or multiplayer-production claim. The hosting process remains responsible for authenticating the principal before calling `Start`; this adapter only makes the resulting principal binding explicit on the existing loopback path.
