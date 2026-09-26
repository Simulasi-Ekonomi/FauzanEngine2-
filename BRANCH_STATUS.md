# BRANCH STATUS — p4-release-certification-night

**Audit date:** 2026-09-26
**Branch:** p4-release-certification-night
**Current compare with main:** 212 commits ahead / 245 behind; diverged
**Merge-base:** 580f009ecbde0aa66f0334071064cd0d3adce9a3
**Active PR:** none

## Current findings

- P4 release-certification tooling is present: release manifest, source SBOM, reproducibility, artifact, and provenance gates.
- Canonical CMake currently excludes `Systems/ItemSerialTracker.cpp`.
- Workflow inventory is currently preserved relative to `main`; no upstream CI deletion was observed in the current branch tree. Older notes claiming broad workflow deletion are historical/stale and must not be used as current evidence.
- Android Release packaging was hardened to fail closed: keystore material is required, Release APK+AAB are built, and APK signing is verified with `apksigner`.
- No exact-head P4 CI certification evidence is currently established.
- The branch remains highly diverged from current `main`; no promotion or merge decision is justified from this branch state alone.

## Required evidence before promotion

- Reconcile the P4 work against the current main baseline without destructive wholesale merge.
- Run the P4 release-certification workflow on the exact final SHA.
- Validate release manifest and source SBOM against the exact HEAD/tree.
- Validate signed APK/AAB artifacts and provenance/reproducibility.
- Run applicable Release/ASAN/integration gates on the same revision.
- Re-audit canonical runtime scope, duplicate source trees, and workflow preservation immediately before any merge.

## Status

**BLOCKED / UNVERIFIED**

This file records current evidence only. Commit count, source count, or tool presence does not change certification status.
