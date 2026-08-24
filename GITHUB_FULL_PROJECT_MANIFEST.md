# Full FauzanEngine GitHub Storage Manifest

This manifest interprets **"entire FauzanEngine and all code"** as all reviewable project source, tests, contracts, tooling, configuration, and documentation under the FauzanEngine workspace. It does **not** include generated build products, package dependencies, local device/configuration files, private memory/vault data, credential-bearing configuration, or backup archives.

> The remote target will be the new public repository, [`Simulasi-Ekonomi/FauzanEngine2-`](https://github.com/Simulasi-Ekonomi/FauzanEngine2-). The trailing hyphen is part of the actual repository name shown in the user-provided GitHub screenshot. This clean repository intentionally avoids mixing the complete current workspace with the materially older Devin-based `FauzanEngine` history. Its first branch must be `main`.

## Reviewed full-project candidate scope

| Include | Reason |
|---|---|
| `Source/`, `Tests/` | Canonical C++ source and executable smoke evidence. |
| `engine/` | Historical/legacy engine source already present in the repository; retain it rather than silently discarding it. |
| `backend/`, `editor/`, `android/`, `jni/` | Existing project code and platform integrations, excluding ignored local Android/device output. |
| `World/`, `Systems/`, `Assets/`, `Agents/`, `skills/`, `tools/`, `.github/` | Project assets, agent/skill definitions, tools, and workflow metadata. |
| Root source/configuration/docs/scripts | Project records, contracts, status files, CMake/tooling files, README, TODO, and GitHub preparation material. |

## Explicit exclusions

| Exclude | Reason |
|---|---|
| `build/`, `Builds/`, `bin/`, `build_bench/`, `build_bench_linux/`, `node_modules/`, `editor/dist/`, `android/.gradle/`, `APKTest/`, `benchmark/`, `benchmark_standalone/`, `litert_libs/` | Generated build, benchmark, test-package, dependency, or Gradle cache output. Some of these paths are historically tracked in the old local repository and therefore require the clean staging-tree procedure below; `.gitignore` alone cannot remove an already tracked path. |
| `Coba_Memory/`, `Coba_Obsidian_Vault/`, `Aries_Subconscious_Vault.db`, backup archives | Private/local memory, database, or backup material rather than project source. |
| `.claude/`, `.claude-flow/`, `.swarm/`, `.worktrees/`, `.mcp.json`, `*.env` | Local agent/session/worktree state, connector configuration, or environment credentials. |
| `android/local.properties`, `android/app/src/main/jniLibs/` | Machine-specific Android configuration or generated binary payload. |
| `*.so`, `*.spv`, `*.tmp`, `*.bak*`, `*.V*_backup`, `.trashed-*`, `*.db`, `*.tar.gz`, `__pycache__/`, logs and transient test-event files | Compiled binaries, generated shader output, temporary/backup/trash data, local database/archive, Python cache, or ephemeral diagnostics. |

## Recommended branch and commit procedure

1. Use the already-created empty public repository `Simulasi-Ekonomi/FauzanEngine2-` with no starter README, license, or `.gitignore`.
2. Build a new local staging tree from the workspace using the exclusions in this manifest. This clean tree prevents paths historically tracked by the old Devin/local repository—such as `android/.gradle/`—from entering the new initial commit. Do not use a destructive cleanup command against the working workspace.
3. Initialize `main` only inside that staging tree, stage the reviewed scope, and inspect `git status --short` plus a filename-only secret scan before commit. The source snapshot intentionally retains pre-existing trailing whitespace across historical files; record the result of `git diff --cached --check` as a non-blocking historical-format exception rather than autoformatting thousands of unreviewed lines during archival.
4. Make the initial `main` commit with a message that identifies this as a complete FauzanEngine workspace baseline and records the then-current smoke baseline. Later archival updates must compare the full local reviewable tree with `git ls-files`, document every remaining gap, and never introduce engine features as a substitute for missing source archival.
5. Push only after the user explicitly confirms the `main` branch, public visibility, staged scope, commit, and remote push.

No remote branch, commit, or push is created by this document.
