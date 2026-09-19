# FauzanEngine2- — CI Runtime Dependency Baseline (September 2026)

**Canonical branch at time of this record:** `p3-pr44-advanced-port`  
**Purpose:** prevent future rooms/agents from silently downgrading the CI/runtime dependency baseline.

## 1. Mandatory baseline

| Component | Required baseline |
|---|---|
| Node.js | **24.x LTS** |
| Node.js current reference | 26.x exists, but CI standard remains Node 24 LTS for production reproducibility |
| SDL | **SDL3 3.4.16** |
| C++ build family | C++23 where the target already requires it |
| Runner | Ubuntu 24.04 for Vulkan/runtime workflows where specified |

Node.js 24.21.0 is an LTS release as of September 2026. Node.js 26.9.0 is the Current line. Do not replace the CI baseline with Node 20.

SDL 3.4.16 is the current stable SDL3 release used by this repository's CI baseline. Do not restore SDL 3.4.14 or SDL2 in active production workflows.

## 2. GitHub Action runtime baseline

Active workflows must use Node-24-compatible action generations:

- `actions/checkout@v7`
- `actions/setup-node@v7`
- `actions/setup-python@v7`
- `actions/setup-java@v6`
- `actions/upload-artifact@v6`
- `gradle/actions/setup-gradle@v6`
- `android-actions/setup-android@v4` (Node 24 runtime)

Where Node is explicitly installed for JavaScript tooling:

```yaml
uses: actions/setup-node@v7
with:
  node-version: '24'
```

## 3. SDL baseline

For workflows using `libsdl-org/setup-sdl`:

```yaml
uses: libsdl-org/setup-sdl@main
with:
  version: 3.4.16
  install-linux-dependencies: true
```

For workflows that download SDL manually, the URL, extracted source directory, checksum, and CMake source path must all refer to **3.4.16**.

SDL 3.4.16 source SHA-256:

```
7322236cd12090c3eb40b9728be4d49c76f66ad17d04369584d4ecad5cf77c68
```

## 4. No partial migration

A dependency migration is complete only when all active workflows using that dependency agree on the same baseline.

Do **not**:
- update P0 but leave R1–R12 on an older SDL version;
- update one manual SDL download while another workflow still downloads an older release;
- set Node 24 in one JavaScript workflow while another active workflow still explicitly installs Node 20;
- downgrade dependencies merely because a previous green run used the older baseline;
- change only one layer of a multi-layer dependency setup.

When a failure occurs after this migration, diagnose it against the new baseline rather than silently reverting one workflow.

## 5. Production-rule interaction

This baseline does not authorize weakening engine/runtime tests.

CI failures must be classified as:
1. dependency/toolchain compatibility failure;
2. workflow configuration failure;
3. test-harness failure;
4. engine source regression;
5. platform/runtime limitation.

The failing layer must be repaired without deleting or weakening the underlying production contract.

## 6. Historical migration note

The repository previously mixed SDL 3.4.14 and SDL 3.4.16 across workflows. That mixed state is explicitly retired.

The repository also contained explicit Node 20 setup in the web/editor and Android workflows. Those explicit pins are retired in favor of Node 24 LTS.

## 7. Room/agent handoff rule

Before changing CI dependencies:

1. Read this document.
2. Read `docs/ENGINE_PRODUCTION_EXECUTION_RULES.md`.
3. Inspect every active workflow under `.github/workflows/`.
4. Preserve the **Node 24 + SDL 3.4.16** baseline unless the user explicitly authorizes a new migration.
5. If a new migration is authorized, update the entire active workflow/test surface consistently and update this document.

**Golden rule:**  
> **Do not downgrade Node or SDL in one workflow to make an isolated CI failure disappear. Keep the system on one explicit baseline and fix the actual incompatibility.**
