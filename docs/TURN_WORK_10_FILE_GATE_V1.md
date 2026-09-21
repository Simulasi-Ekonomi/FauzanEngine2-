# Turn Work Gate — 10 Real Files Per Branch

**BINDING.** Each turn must deliver at least 10 distinct real source files on this branch. One file with many guards/checks counts as one file. Test-only/docs-only/formatting/duplicate guards do not count. Every counted file must map Gap ID → file → API/function → implementation → integration path → validation result. If fewer than 10 independently actionable gaps remain, report the verified count instead of inventing work. This document itself never counts toward the 10.

## Turn ledger
| # | Gap ID | File | Implementation/integration | Validation |
|---:|---|---|---|---|
|1|OPEN|TBD|TBD|TBD|
|2|OPEN|TBD|TBD|TBD|
|3|OPEN|TBD|TBD|TBD|
|4|OPEN|TBD|TBD|TBD|
|5|OPEN|TBD|TBD|TBD|
|6|OPEN|TBD|TBD|TBD|
|7|OPEN|TBD|TBD|TBD|
|8|OPEN|TBD|TBD|TBD|
|9|OPEN|TBD|TBD|TBD|
|10|OPEN|TBD|TBD|TBD|

No merge/100% claim from commit count or line count. Release/ASAN failures remain blockers.

## Mandatory execution rule — no idle turns
- When the user says **Lanjut**, immediately execute identified real gap work; do not spend the turn on explanations or repeated scans when actionable source/API evidence is already available.
- No delay or workflow-only substitute for implementation. If a real gap is known, implement and integrate it.
- If fewer than 10 real gaps remain on a branch, close every remaining real gap, then immediately proceed to tests, CI, sandbox/release gates, and merge readiness as applicable.
- Genuine gaps found in `main` may be ported to the active branch; never fabricate accounting items.
- The 10-distinct-real-source-files gate remains mandatory whenever at least 10 independently actionable gaps exist.
- Test/docs/formatting/duplicate guards do not count. Implementation must be integrated and validated; otherwise status is IMPLEMENTED-UNVERIFIED.
