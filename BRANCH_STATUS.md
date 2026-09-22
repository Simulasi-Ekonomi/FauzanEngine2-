# BRANCH STATUS — p4-release-certification-night

**Audit date:** 2026-09-22  
**Branch:** p4-release-certification-night  
**Compare with main:** 175 commits ahead / 67 behind; diverged  
**State:** no active PR located

## Critical audit finding
The branch comparison shows deletion of multiple existing CI workflows, including:
- P0 sandbox validation
- Android build
- web build
- networking smoke
- PBR validation
- R1/R2/R3/R4/R5/R6/R8/R9 workflows
- renderer 3D smoke

Those deletions are not certification evidence and must not be used to make CI green. They must be reviewed before any promotion/merge.

## Added P4 tooling
The branch adds release-manifest, source-SBOM, reproducibility, artifact and provenance gates. These are useful certification infrastructure, but their presence does not prove release certification.

## Required evidence
- Preserve/restore required upstream CI coverage before certification.
- Validate release artifact gates against a real canonical build.
- Run Release + ASAN/integration/regression suites on one exact revision.
- Verify reproducible packaging/signing, migration/recovery, device/runtime gates and provenance.
- Do not mark P4 100% from scripts or documentation alone.

## Documentation rule
This file is authoritative for the current branch audit. Older release documents containing unsupported readiness claims must be rewritten as historical records or removed.
