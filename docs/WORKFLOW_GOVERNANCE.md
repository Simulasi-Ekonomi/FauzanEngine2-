# WORKFLOW GOVERNANCE — FAUZANENGINE2-

## Purpose

This document defines the permanent workflow/CI rules for FauzanEngine2-.

These rules apply to every future ChatGPT/agent room working on this repository.

## 1. Repository Preservation Rule

**NEVER delete a workflow from the repository merely because a branch does not need it.**

A workflow may be absent from a feature/workstream branch when it is not relevant to that branch.

This means:

- Repository history must retain workflows.
- `main` is the canonical workflow set.
- A branch may carry only the workflows required for its workstream and merge protection.
- Removing a workflow from a branch is NOT permission to delete its repository history.
- Do not rewrite or destroy historical workflow definitions.

## 2. Main Branch Rule

`main` must contain the complete production workflow/gate set required by the project.

`main` is the final integration and certification target.

A workflow belongs on `main` when it protects any production requirement, integration boundary, security requirement, platform requirement, runtime requirement, or release requirement.

## 3. Feature Branch Rule

Feature/workstream branches should contain only workflows that are relevant to that branch.

However, **"relevant" does NOT mean only files directly edited by the branch.**

A workflow is relevant when it:

1. validates code being changed by the branch;
2. validates a dependency used by the branch;
3. protects a requirement that the branch could regress;
4. protects a requirement needed before merging into `main`;
5. prevents downgrade, substitution, or architectural regression;
6. validates an integration boundary shared with another active workstream.

Therefore, never remove a workflow solely because it belongs to another subsystem.

## 4. Protection-Gate Principle

A branch must be prevented from introducing regressions that would violate `main`.

Example:

### SDL3

SDL3 is a project requirement.

Therefore `sdl3-modernize.yml` is relevant to renderer/audio/input work even if the branch is not performing the SDL migration itself.

It protects against:

- accidental SDL2 reintroduction;
- stale SDL2 headers/API;
- SDL2 CMake linkage;
- incompatible renderer/audio/input changes;
- downgrade of the SDL3 architecture.

**Do not remove the SDL3 guard from a branch that can affect SDL/platform/runtime code.**

## 5. Workflow Classification

Before changing any workflow, classify it:

### KEEP

The workflow is directly relevant to the branch or protects a requirement the branch can affect.

Keep it active.

### REWIRE

The workflow is useful, but its trigger still references an obsolete branch, old feature branch, or obsolete PR structure.

Update its trigger/path/target to the current workstream.

Do not delete the workflow.

### BRANCH-EXCLUDE

The workflow is valid in the repository/main but is not required on the current feature branch.

It may be absent from that branch.

The workflow must remain preserved in repository history/main.

### PAUSED

The workflow is intentionally not part of the current development phase.

Current example:

- `build-android.yml`
- `build-web.yml`

These remain preserved and are activated when the project enters the final build/certification phase.

### RETIRE

Retire means obsolete as an active gate.

It does **NOT** mean erase repository history.

If a workflow is retired, preserve its history and do not remove it from historical commits.

## 6. Trigger Audit Rule

Before editing a workflow, inspect:

```
workflow
→ trigger
→ branch
→ PR target
→ path filters
→ workstream
→ main requirement protected
→ current status
```

Never change a workflow based only on its filename.

For every workflow under consideration, answer:

- Which branch owns this workflow?
- Which code does it validate?
- Which requirement does it protect?
- Does the current branch affect that requirement?
- Is its trigger still valid?
- Is the target branch still valid?
- Is it a current gate or historical workflow?

## 7. Merge Gate Rule

`mergeable=true` is **NOT** equivalent to validation PASS.

Before merge, the applicable sequence is:

```
IMPLEMENTATION
→ INTEGRATION
→ VALIDATION
→ SANDBOX
→ MERGE
```

A branch is not merge-ready merely because GitHub reports it as mergeable.

Required validation must actually run and pass.

Never claim PASS without workflow/run evidence.

## 8. Do Not Game CI

Do not make CI green by:

- deleting tests;
- deleting functionality;
- weakening assertions;
- removing targets;
- replacing real implementations with stubs;
- reducing workload;
- bypassing sanitizer coverage;
- disabling required gates;
- downgrading SDL3 to SDL2;
- hiding compiler/runtime errors.

CI must prove the real implementation.

## 9. Branch-Specific Workflow Rule

For active workstreams:

### P1 — Renderer / Asset / Animation

Relevant protection includes:

- renderer 3D validation;
- GPU indirect execution;
- PBR validation;
- asset/GPU ownership;
- animation/GPU skinning;
- SDL3 protection;
- applicable sandbox validation.

### P2 — Physics / Networking

Relevant protection includes:

- XPBD/physics validation;
- networking/replication;
- prediction/reconciliation;
- authoritative transport;
- performance/profiling;
- shared runtime protection required by `main`.

### P3 — Editor / Android / Production

Relevant protection includes:

- editor/runtime;
- Android platform;
- audio/input where affected;
- telemetry;
- persistence/recovery;
- security/privacy;
- signing/launch gates;
- sandbox validation.

### PR44 Advanced Port

This is an independent mandatory workstream.

Relevant validation includes:

- editor agent/session;
- audio/WAV/pitch/resampling;
- HiZ/occlusion;
- PBR/normal-map capability;
- regression smoke;
- all dependencies required for integration.

### P4 — Release Certification

Relevant validation includes:

- deterministic release manifest;
- source SBOM;
- hash integrity;
- signing-material protection;
- release gate;
- security/release integrity.

## 10. Build Phase Rule

Final Android/Web build workflows are intentionally PAUSED during the current implementation/validation phase.

Do not spend development time repeatedly fixing final-build workflows before the project reaches the build phase.

When build phase begins, reactivate and validate them against the current `main`.

## 11. Workflow Changes Must Be Evidence-Based

Before modifying a workflow:

1. Read its current content.
2. Check its current branch/trigger.
3. Check the owning workstream.
4. Check whether the requirement is still active.
5. Check current workflow runs.
6. Compare against `main`.
7. Only then modify it.

Never infer that a workflow is obsolete from its filename alone.

## 12. No Destructive Cleanup

Do not perform bulk workflow deletion.

Do not remove multiple workflows merely to make a branch look clean.

Branch cleanliness means:

> only the workflows necessary for that branch are active/present on that branch, while the repository retains the complete historical record and `main` retains the production gate set.

## 13. Room Handover Rule

Any new ChatGPT room working on FauzanEngine2- must read this document before modifying CI/workflows.

The room must preserve these principles:

- no workflow history destruction;
- `main` is the canonical complete workflow target;
- feature branches carry relevant workflows only;
- protection gates count as relevant;
- SDL3 protection must not be removed from affected branches;
- build workflows remain paused until build phase;
- no fake PASS;
- no CI gaming;
- no destructive cleanup.

## 14. Current Project Priority

Workflow maintenance must not consume the entire development session.

After necessary workflow correction:

1. return to substantive P1/P2/P3/PR44/P4 implementation;
2. run validation where applicable;
3. inspect actual failures;
4. fix real blockers;
5. sandbox;
6. merge only after evidence supports it.

**CI exists to protect the engine, not to become the engine-development task itself.**
