# R5 — Asset Streaming & LOD

**Document status:** HISTORICAL / SUPERSEDED  
**Synchronized:** 2026-09-22

This file records the former R5 plan and implementation intent. It is **not** a current production certification.

Claims in the old version such as “SHIPPED”, “Production Ready”, “95%+ Unreal parity”, projected 100K performance and broad Release/ASAN success were not retained as current evidence because they were tied to older checkpoints and are not sufficient to certify the current branch.

## Current acceptance boundary
Asset streaming is considered complete only when the canonical importer → staging → resource ownership → streaming/eviction → GPU upload → renderer/animation consumer path is integrated and validated on an exact revision with Release + ASAN evidence and applicable runtime/device tests.

## Historical use
Keep this document only as historical planning context. Current status belongs in BRANCH_STATUS.md and the current master roadmap.
