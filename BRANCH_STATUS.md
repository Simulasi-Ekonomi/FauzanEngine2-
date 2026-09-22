# BRANCH STATUS — p2-physics-networking-night

**Audit date:** 2026-09-22  
**PR:** #78 — SANDBOX validation  
**HEAD:** e3bfa8eaa43b320ce0ce9126832eb772eb0bb937  
**Base:** sandbox-p0-validation-20260920 @ d12754fe619a1171ea5ad86a040da203066c8b34  
**State:** OPEN / mergeable=false

## Evidence status
- This branch is not merge-ready.
- Fresh GitHub workflow query for HEAD returned no workflow runs; exact-HEAD CI/ASAN/sandbox PASS is therefore unverified.
- PR changed 55 files, including canonical XPBD, GameplayPhysicsQuery, CanonicalReplicationBridge, ReplicationWorld, authoritative session, CMake and runtime smokes.
- todo.md is a global backlog/ledger and is not a branch-gap counter.

## Real audit focus
1. XPBD authoritative ECS/gameplay ownership and pose synchronization.
2. 100K bodies / at least 200K collision tests / strictly under 5 ms measured benchmark.
3. Replication authority, snapshot/delta, reconnect and failure semantics.
4. Physics query correctness and transactional mutation behavior.
5. Canonical CMake/source registration and Release + ASAN evidence.
6. Duplicate legacy source paths must not be counted as canonical capability.

## Blocking evidence
- The acceptance performance target is not proven by contract/source existence.
- Exact-HEAD CI/ASAN/sandbox evidence is missing.
- Production networking transport, prediction, reconciliation, interest management and reconnect evidence remains to be established.

## Documentation rule
Only current evidence may drive readiness percentages. Historical plans remain historical and must not be presented as certification.
