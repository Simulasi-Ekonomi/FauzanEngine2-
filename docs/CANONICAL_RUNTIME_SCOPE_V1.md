# Canonical Runtime Scope V1

## Purpose

This document defines the **build-scope boundary** for the canonical C++23 NeoEngine runtime. It prevents the repository from presenting legacy, experimental, or imported code with capability markers as an implemented runtime feature merely because the file remains in the source tree.

The canonical source of truth is `Source/NeoEngine/CMakeLists.txt`, specifically `XPBD_RUNTIME_SOURCES`. `tools/verify_canonical_runtime_scope.sh` parses that list and compares every source/header containing a tracked marker—`TODO`, `FIXME`, `NOT_IMPLEMENTED`, `NotImplemented`, `placeholder`, `Placeholder`, `stub`, or `Stub`—against `tools/canonical_runtime_scope_manifest_v1.txt`. The verifier fails closed when a marker has no classification, a listed file disappears, a manifest path duplicates another path, a non-active marked `.cpp` is admitted to `XPBD_RUNTIME_SOURCES`, or the approved active boundaries change.

## Active Canonical Entry Points

The canonical list compiles `Runtime/NeoRuntime.cpp`, `Runtime/VulkanPresentProbe.cpp`, and `Runtime/FarmRuntimeSession.cpp`. These are active implementation seams, not a statement that the complete renderer, Vulkan RHI, Farm networking, platform delivery, or production lifecycle is complete.

Two marked active paths are deliberately constrained.

| Active path | Classification | Enforced interpretation |
|---|---|---|
| `Core/EngineLoop.cpp` | `active_fail_closed_legacy` | Every public lifecycle entry throws the explicit `NOT_IMPLEMENTED` error and directs callers to `NeoRuntime`; it must never be represented as a validated render loop. |
| `Runtime/RendererCapability.h` | `active_enum_marker` | `NotImplemented` is an error-state enumerator for explicit capability reporting, not an implementation claim. |

## Non-Active Classifications

The manifest classifies every remaining current marker path. `non_active_legacy` files are retained historical or incomplete source that must not be added to the canonical source list without a separate implementation and proof increment. `non_active_experimental` files are non-canonical prototypes, including V4 AI/RL headers. `non_active_fail_closed_legacy` identifies the old `Rendering/RHI/Vulkan/VulkanRHI.cpp` boundary, which is intentionally excluded and must remain fail-closed rather than impersonating device success. `third_party_upstream_marker` identifies TODO/FIXME comments from the imported TinyObjLoader upstream source; those comments do not certify an engine capability.

The full path-level classification is versioned in `tools/canonical_runtime_scope_manifest_v1.txt`; it covers AI/OpenCode and V4 RL headers, old character/ECS/editor paths, Android platform placeholder code, legacy transform/memory/streaming/item paths, the legacy Vulkan RHI, the old rendering frame graph header, and upstream TinyObjLoader markers.

## Claim Boundary

Passing the verifier proves only that the observed marker inventory is explicitly classified relative to the current CMake runtime list. It does **not** prove that all canonical sources have no defects, that unmarked modules are production-ready, or that excluded modules are deleted. It does **not** close P2.1, P0.3, P0.4, P0.5, P0.6, or any release-readiness gate.

In particular, this boundary does not support claims of a production Vulkan renderer, physical-device/device-loss coverage, authoritative network multiplayer, provider-backed payment processing, Android/Play release readiness, autonomous agent deployment, or Unreal parity. Those claims require independent code paths and executable evidence.

## Reproduction

From repository root, run:

```bash
tools/verify_canonical_runtime_scope.sh
tools/canonical_runtime_scope_smoke.sh
```

The verifier is repository-local and has no network or persistent source mutation behavior. The adversarial smoke temporarily creates a marked C++ probe, requires the verifier to reject it as unclassified, removes the probe, and requires the clean verifier to pass again. Pair those script passes with focused canonical runtime smoke tests in both Release and AddressSanitizer configurations; a classifier pass is not memory-safety evidence by itself.
