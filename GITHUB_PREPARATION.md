# GitHub Storage Preparation

This checklist is for storing the current **single-segment skeletal-route baseline** and the wider reviewed FauzanEngine workspace in the new GitHub repository, `Simulasi-Ekonomi/FauzanEngine2-`. It is not a release checklist and it does not authorize publishing, deployment, or a production-readiness claim.

1. Keep the canonical code path under `Source/NeoEngine` and canonical executable evidence under `Tests`.
2. Keep the project-facing overview in `README.md`, the documentation map in `docs/README.md`, and historical work in `todo.md`.
3. Keep the readiness status in `production_backend_readiness.md` as **NOT PASSED**.
4. Do not add generated build output, dependency directories, local environments, private memory stores, or agent runtime scratch data to the repository.
5. Build both maintained configurations and run all eligible non-Vulkan smoke executables directly, with `ASAN_OPTIONS=detect_leaks=1` for AddressSanitizer.
6. Run the scoped `git diff --check` command in `docs/README.md` and review `git status --short` before staging files.
7. Follow `GITHUB_FULL_PROJECT_MANIFEST.md` for the reviewed full-project scope and exclusions. This includes the canonical code/test baseline plus reviewed legacy engine, backend, editor, Android, JNI, agent/skill, tools, assets, root contracts, documentation, and repository metadata. Do not use destructive cleanup commands.

```bash
# Prepare only a disposable clean staging tree; do not alter the current workspace index.
source_dir=/home/ubuntu/work/fauzan_engine/src/FauzanEngine
stage_dir=/tmp/fauzanengine2-initial-review
rm -rf "$stage_dir"
mkdir -p "$stage_dir"
cd "$source_dir"
tar --exclude-vcs \
  --exclude=build --exclude=Builds --exclude=bin --exclude=build_bench \
  --exclude=build_bench_linux --exclude=node_modules --exclude=APKTest \
  --exclude=benchmark --exclude=benchmark_standalone --exclude=litert_libs \
  --exclude=editor/dist --exclude=android/.gradle --exclude=android/local.properties \
  --exclude=android/app/src/main/jniLibs --exclude=Coba_Memory \
  --exclude=Coba_Obsidian_Vault --exclude=Aries_Subconscious_Vault.db \
  --exclude=Coba_Memory_PASS_282_backup.tar.gz --exclude=.claude \
  --exclude=.claude-flow --exclude=.swarm --exclude=.worktrees --exclude=.mcp.json --exclude='*.env' \
  --exclude='*.so' --exclude='*.spv' --exclude='*.tmp' --exclude='*.bak*' \
  --exclude='*.V*_backup' --exclude='.trashed-*' --exclude=__pycache__ \
  --exclude='*.db' --exclude='*.tar.gz' --exclude='report_part_*' \
  --exclude='*_report*.txt' --exclude='aries_chaos_log_*.txt' \
  --exclude='test_event*.txt' --exclude=watcher_test.txt \
  -cf - . | tar -xf - -C "$stage_dir"
cd "$stage_dir"
git init -b main
git add -A
# Record historical whitespace warnings; do not mass-reformat archival code.
git diff --cached --check || true
git status --short
```

The historical `engine/` tree is deliberately included in the FauzanEngine2 full-project baseline. Generated build/dependency output, local memory/vault/database/backup data, credentials, machine configuration, binaries, shader output, and transient diagnostics remain excluded by the clean staging procedure and the manifest.

The current baseline intentionally excludes multi-segment skeletal routing, root rotation, kinematic fallback or delta blending, NPC behavior, collision/physics coupling, prediction, multiplayer, renderer binding, deployment, and production readiness.
