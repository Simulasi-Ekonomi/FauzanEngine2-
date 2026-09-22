# AI HANDOVER PROTOCOL — FauzanEngine2-

## Purpose
Canonical operating protocol for coding rooms and agents continuing repository work.

## Canonical repository
- Repository: Simulasi-Ekonomi/FauzanEngine2-
- Canonical source: Source/NeoEngine
- Canonical project path: /storage/emulated/0/Buku saya/FauzanEngine
- main is protected during branch work.
- Current audited branch: p3-editor-android-production-night
- Sandbox PR: #79

## Non-negotiable rules
1. READ the exact current file and call sites before EDIT.
2. Preserve real functionality; upgrade rather than downgrade.
3. No mass stubs, fake implementations, placeholder production code or duplicate authority paths.
4. Never invent paths, APIs, tests, benchmarks, CI results or readiness.
5. Never overwrite blindly; use current file content/SHA.
6. Every source change must be followed through API → callers → ownership/lifetime → data flow → integration → canonical CMake → runtime path → test.
7. CI PASS is not Termux/device/runtime proof.
8. Do not merge until implementation, integration, Release, ASAN, sandbox and required runtime/device evidence all support MERGE-READY.
9. A percentage may not increase merely because code or documentation was added.
10. Historical branches and documents are evidence sources, not automatic authority.

## Required workflow
READ → ANALYZE → EDIT → BUILD/CI → TEST → BENCHMARK → REVIEW → COMMIT → PUSH → SANDBOX → MERGE → POST-MERGE VERIFY.

## Performance contract
XPBD acceptance remains 100,000 bodies, at least 200,000 collision tests and measured physics step strictly under 5 ms. Contract/source presence is not benchmark evidence.

## Documentation policy
- BRANCH_STATUS.md is the current branch checkpoint.
- todo.md is a backlog/implementation ledger, not a gap counter.
- Historical R-series documents must be labeled historical/superseded when their old claims no longer describe the current revision.
- No document may claim production readiness, Unreal parity, benchmark PASS or 100% solely from source presence, projected metrics or old CI.

## Verification labels
- VERIFIED-CI: exact workflow passed on stated revision.
- VERIFIED-TERMUX: user/device execution passed on stated revision.
- VERIFIED-BENCH: measured workload and timing passed.
- IMPLEMENTED-UNVERIFIED: implementation exists but required runtime evidence is missing.
- CONTRACT-ONLY: contract/test exists without proof of target behavior/performance.
- BLOCKED: concrete dependency prevents safe continuation.
