# FAUZANENGINE — AGENT ENGINEERING PROTOCOL

**Canonical policy for every coding agent working on FauzanEngine2-**

Status: ACTIVE  
Scope: MAIN + ALL FEATURE/RELEASE/SANDBOX BRANCHES  
Applies to: Codex, Antigravity, Claude, Gemini, Qwen, local agents, and any other coding agent

---

## 1. Core Principle

Every agent must work using an **implementation → integration → verification → correction** loop.

Passing compilation or an isolated smoke test is **not** sufficient evidence that a capability is complete.

The agent must prove the complete runtime path and must continue working when verification exposes a defect.

Required loop:

```
SCAN
  ↓
UNDERSTAND EXISTING IMPLEMENTATION
  ↓
IDENTIFY CONCRETE GAP
  ↓
IMPLEMENT REAL PRODUCTION CODE
  ↓
INTEGRATE WITH THE REAL ENGINE PATH
  ↓
BUILD
  ↓
TEST
  ↓
INSPECT FAILURES / DEFECTS
  ↓
CORRECT IMPLEMENTATION OR TEST WIRING
  ↓
REBUILD
  ↓
RETEST
  ↓
RELEASE + ASAN + SANDBOX VALIDATION
  ↓
DOCUMENT EXACT EVIDENCE
```

Do not stop merely because the first implementation compiles.

---

## 2. Zero-Gap Engineering Rules

The following are mandatory:

- **ZERO STUB**
- **ZERO PLACEHOLDER**
- **ZERO DUMMY IMPLEMENTATION**
- **ZERO FAKE SUCCESS**
- **ZERO MINIMAL IMPLEMENTATION used to satisfy a test**
- **ZERO DISCONNECTED IMPLEMENTATION**
- **ZERO UNEXERCISED CRITICAL PATH**
- **ZERO UNVERIFIED CLAIM OF COMPLETION**

Code must be functional, integrated, maintainable, and production-oriented.

Do not replace missing functionality with:
- empty functions;
- hard-coded success;
- fake return values;
- test-only bypasses;
- disabled code paths;
- comments claiming future implementation;
- artificial smoke-test passes;
- reduced requirements merely to make CI green.

If a capability is incomplete, report it as incomplete and continue closing the gap.

---

## 3. Scan Before Editing

Before changing code:

1. Inspect the relevant branch and commit state.
2. Read existing implementation and adjacent interfaces.
3. Search for all callers, owners, lifecycle points, and tests.
4. Inspect existing CMake targets and validation manifests.
5. Check related documentation and previous gap reports.
6. Determine whether another branch already owns overlapping work.

Never overwrite or redesign an existing implementation blindly.

Preserve working behavior unless the requested gap requires a change.

---

## 4. Work on the Real Runtime Path

Every capability must be traced through its actual lifecycle.

Example:

```
input / bytes
 → import
 → dependency resolution
 → cache / staging
 → resource ownership
 → runtime object
 → scene integration
 → renderer
 → GPU
 → presentation / observable result
```

For streaming:

```
file request
 → queue
 → priority scheduling
 → async I/O
 → cancellation
 → resource ownership
 → upload
 → residency
 → refresh notification
 → renderer consumption
 → eviction / retry
```

Do not declare a subsystem complete when only one isolated component works.

---

## 5. Tests Must Exercise Real Behavior

A smoke test must test the actual implementation.

When a test fails:

1. Determine whether the implementation is wrong.
2. Determine whether integration is wrong.
3. Determine whether test wiring is wrong.
4. Fix the actual defect.
5. Re-run the exact target.

Do **not** weaken assertions or alter expected behavior simply to obtain a green result.

A useful agent must be willing to discover that its own newly written test is incorrectly wired and fix that wiring, rather than assuming the implementation is at fault.

---

## 6. Required Validation Layers

For relevant production work, use the following progression:

### Layer A — Static integrity
- source inspection
- API/lifecycle inspection
- dependency/call-site inspection
- `git diff --check`

### Layer B — Build
- CMake configure
- complete Release build
- complete ASAN build where supported

### Layer C — Runtime
- dedicated smoke/integration test
- regression tests
- renderer/runtime validation where applicable

### Layer D — Memory safety
- ASAN
- leak detection
- sanitizer-specific runtime paths

### Layer E — Sandbox / parity
- exact sandbox target
- production-like execution
- capability-specific acceptance

### Layer F — Device / scale
When required:
- physical-device validation
- device-loss recovery
- target-device performance
- scale/load acceptance

Passing Layers A–E does not automatically prove Layer F.

---

## 7. Release and ASAN Parity

Every new production capability that belongs to the release gate must be represented consistently in:

- CMake;
- Release target manifest;
- ASAN target manifest;
- dedicated test coverage;
- sandbox validation where applicable.

A target existing only in Release or only in ASAN is insufficient when both are required.

Prefer identical source/input semantics between Release and ASAN.

---

## 8. Evidence, Not Assertions

Every completion report must contain:

- branch;
- exact HEAD SHA;
- implementation commits;
- files/subsystems changed;
- capability completed;
- tests added/changed;
- exact validation commands or CI runs;
- Release result;
- ASAN result;
- sandbox result;
- remaining gaps;
- known limitations;
- whether physical-device evidence exists.

Use explicit status:

- `PASS`
- `PARTIAL`
- `OPEN`
- `BLOCKED`
- `UNVERIFIED`

Never write "production ready", "zero gap", or "complete" when any required capability or evidence remains open.

---

## 9. Discovery of New Defects Is Progress

If implementation exposes a new defect, that defect must be recorded and corrected.

Example:

```
implementation
 → smoke test
 → callback missing
 → identify test/implementation wiring defect
 → fix wiring
 → rerun
 → PASS
```

Finding a defect during validation is **not a reason to hide or bypass it**. It is part of the engineering loop.

---

## 10. Integration Is Mandatory

A file being implemented does not mean the feature is implemented.

For each subsystem, verify:

- producer;
- consumer;
- ownership;
- lifetime;
- synchronization;
- error propagation;
- cancellation;
- retry;
- cleanup;
- resource release;
- runtime notification;
- downstream consumption.

Where applicable, test both success and failure paths.

---

## 11. No Downgrade

Never reduce the target merely because implementation is difficult.

Do not:
- remove existing functionality to simplify integration;
- lower test coverage to obtain green CI;
- remove performance requirements without explicit approval;
- replace production code with a stub;
- bypass sanitizer failures;
- bypass sandbox failures;
- remove difficult runtime paths from the acceptance target.

If the correct implementation is substantial, implement the substantial solution.

---

## 12. Commit Discipline

Commits should represent real engineering units.

Prefer:

```
scan → implement → integrate → test → verify → commit
```

Avoid:
- meaningless micro-commits;
- documentation-only commits pretending to close code gaps;
- commits that only rename or move code to conceal missing implementation;
- claiming completion based on commit count.

Progress is measured by **closed capabilities and verified behavior**, not number of commits or lines changed.

---

## 13. Agent Handover Protocol

When an agent reaches a usage/time/context limit, it must leave a machine-readable handover containing:

### Repository state
- repository;
- branch;
- base;
- exact HEAD;
- clean/dirty state;
- active PR.

### Completed work
- implementation summary;
- integration points;
- tests;
- exact validation results.

### Remaining blockers
For every blocker:
- identifier;
- subsystem;
- concrete missing behavior;
- affected files;
- required integration;
- required test/evidence.

### Next action
State the **next concrete engineering operation**, not a generic instruction such as "continue P1".

A subsequent agent must be able to continue from the handover without repeating completed work.

---

## 14. Agent Independence

An agent must not assume another agent's implementation is correct merely because:
- a commit exists;
- CI is green;
- a previous report says "complete";
- a test passes.

Read the implementation and verify the runtime path independently.

Independent review is encouraged.

---

## 15. Branch and Merge Policy

Feature work must remain on its designated branch until its acceptance criteria are satisfied.

Before merge:

```
implementation
+ integration
+ dedicated tests
+ Release
+ ASAN
+ sandbox
+ regression
+ required device/scale evidence
+ review closure
= merge candidate
```

Green CI alone is not a merge criterion.

After successful merge and explicit confirmation, obsolete feature branches may be deleted according to project workflow.

---

## 16. Main Branch Is the Policy Source

This document on `main` is the canonical engineering policy.

Every agent must read this file before beginning repository work.

Feature branches may add stricter requirements, but may not weaken this protocol.

If another instruction conflicts with this protocol, preserve the stricter requirement unless the repository owner explicitly changes this policy.

---

## 17. Definition of Done

A capability is DONE only when:

- implementation exists;
- implementation is real, not stubbed;
- integration is complete;
- success and relevant failure paths are exercised;
- dedicated tests exist where required;
- Release passes;
- ASAN passes where required;
- sandbox/parity passes where required;
- device/scale evidence exists where required;
- no known blocker remains for the declared scope;
- exact evidence is recorded.

Otherwise the correct status is **PARTIAL / OPEN / UNVERIFIED**, not DONE.

---

## 18. Mandatory Agent Report

At the end of every work session, report:

```text
BRANCH:
HEAD:
SCOPE:

IMPLEMENTED:
- ...

INTEGRATED:
- ...

VALIDATED:
- CMake:
- Release:
- ASAN:
- Sandbox:
- Device/Scale:

DEFECTS FOUND:
- ...

DEFECTS FIXED:
- ...

REMAINING GAPS:
- ...

NEXT CONCRETE ACTION:
- ...

STATUS:
PASS / PARTIAL / OPEN / BLOCKED / UNVERIFIED
```

This protocol exists to make the engineering process reproducible across agents and to prevent "green CI" from being mistaken for complete production implementation.
