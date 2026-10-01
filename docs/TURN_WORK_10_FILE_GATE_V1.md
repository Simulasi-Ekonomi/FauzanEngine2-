# Turn Work Gate — 10 Real Files Per Branch

Status: **BINDING**

This document changes the turn accounting rule. A turn is not counted as completed work merely because one file contains many guards, assertions, checks, or helper branches.

## Mandatory per-turn delivery
- Minimum **10 distinct real source files** per active branch.
- Each file must close a distinct technical gap or materially integrate an existing capability.
- A test-only change, documentation-only change, duplicate guard, assertion, counter, or formatting change does **not** count as one of the 10.
- One source file with 10 checks counts as **one file**, not ten.
- Every counted file must map to: Gap ID -> exact file -> API/function -> implementation -> caller/integration path -> validation result.
- If a branch has fewer than 10 independently actionable gaps remaining, report the verified number instead of inventing work.
- No merge/readiness/100% claim is allowed from commit count, line count, file count, or CI labels.
- Release and ASAN failures remain blockers; unverified source remains IMPLEMENTED-UNVERIFIED.
- This MD itself is **governance only and never counts toward the 10 files**.

## Required turn ledger
| # | Gap ID | File | Implementation/integration | Validation |
|---:|---|---|---|---|
| 1 | OPEN | TBD | TBD | TBD |
| 2 | OPEN | TBD | TBD | TBD |
| 3 | OPEN | TBD | TBD | TBD |
| 4 | OPEN | TBD | TBD | TBD |
| 5 | OPEN | TBD | TBD | TBD |
| 6 | OPEN | TBD | TBD | TBD |
| 7 | OPEN | TBD | TBD | TBD |
| 8 | OPEN | TBD | TBD | TBD |
| 9 | OPEN | TBD | TBD | TBD |
| 10 | OPEN | TBD | TBD | TBD |

## Stop condition
Do not mark a turn complete until all counted files are actually changed, integrated where required, and their validation status is explicitly recorded.

## Mandatory execution rule — no idle turns
- When the user says **Lanjut**, immediately execute identified real gap work; do not spend the turn on explanations or repeated scans when actionable source/API evidence is already available.
- No delay or workflow-only substitute for implementation. If a real gap is known, implement and integrate it.
- If fewer than 10 real gaps remain on a branch, close every remaining real gap, then immediately proceed to tests, CI, sandbox/release gates, and merge readiness as applicable.
- Genuine gaps found in `main` may be ported to the active branch; never fabricate accounting items.
- The 10-distinct-real-source-files gate remains mandatory whenever at least 10 independently actionable gaps exist.
- Test/docs/formatting/duplicate guards do not count. Implementation must be integrated and validated; otherwise status is IMPLEMENTED-UNVERIFIED.
