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
- When the user says **Lanjut**, immediately execute the identified real gap work. Do not spend the turn explaining, deferring, or asking for permission when the required source/API evidence is already available.
- **No excuse / no delay:** gap discovery and implementation are the work. Do not replace implementation with workflow discussion, generic reports, or repeated scans once actionable gaps are known.
- If fewer than 10 real gaps remain on a branch, close every remaining real gap in that branch; then immediately proceed to tests, CI, sandbox/release gates, and merge readiness as applicable.
- If additional real gaps exist in `main`, they may be ported to the active branch and implemented there; they must be genuine source-level gaps, not fabricated accounting items.
- Per-turn accounting remains **10 distinct real source files per branch whenever at least 10 independently actionable gaps exist**. Never pad the count with documentation, tests alone, duplicate guards, or cosmetic changes.
- Implementation must be integrated into its actual caller/data path and validated. Source edits without integration/validation remain IMPLEMENTED-UNVERIFIED.
