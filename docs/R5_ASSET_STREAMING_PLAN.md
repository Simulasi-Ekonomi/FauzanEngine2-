# R5: Asset Streaming & LOD System — Evidence Status

**Status:** HISTORICAL PLAN / READINESS CLAIMS SUPERSEDED  
**Audit basis:** P1 asset-streaming audit, 2026-09-26  
**Current branch:** `p1-renderer-asset-animation-night`  
**Policy:** Do not claim production readiness, Unreal parity percentage, or end-to-end completion without current executable evidence.

## Why this document was corrected

The previous version claimed R5 was fully shipped, production-ready, 95%+ Unreal parity, and end-to-end integrated. Current P1 source and test evidence does not establish those claims. The historical delivery list is retained below as scope context, but readiness statements are explicitly superseded.

## Current verified capabilities

- **AssetStreamingQueue:** bounded priority scheduling, residency accounting, cancellation, eviction, and explicit GPU-memory release ownership are implemented and covered by smoke tests.
- **StreamManager:** bounded priority file I/O, queued/active cancellation, resident-byte limits, completion reporting, and snapshot reads are implemented and covered by the existing smoke target.
- **AssetResourceManager:** GPU upload lifetime is now explicitly tracked. A live resource is pinned while a GPU upload is in flight; release, eviction, and hot-reload paths cannot invalidate that resource during the upload.
- **VulkanAssetUploader:** resource uploads retain the exact resource handle and publish GPU residency only after an authoritative Vulkan fence reports completion. Mere command recording/submission is not treated as completion.
- **Validation:** a real Vulkan uploader smoke target was added and is included in the P1 Release/ASAN manifest, increasing the canonical target set from 78 to 79.

## Current unresolved integration gaps

1. **StreamManager -> AssetStreamingQueue -> VulkanAssetUploader:** the complete production ownership graph is not yet demonstrated as one runtime path.
2. **GPU completion -> renderer notification/refresh:** authoritative upload completion exists at the uploader/resource-manager seam, but the downstream renderer refresh/notification path is not yet proven end-to-end.
3. **Retry/failure/eviction lifecycle:** individual contracts exist, but the combined asynchronous retry, failure, completion, release, and eviction lifecycle still requires executable integration evidence.
4. **Persistent cache/cook/content invalidation:** not established by the current P1 evidence.
5. **Device/scale acceptance:** physical-device, device-loss recovery, Android/device-scale, and production performance evidence remain open.

## Historical scope delivered by earlier work

The R5 workstream includes these components and concepts:

- StreamManager
- AssetStreamingQueue
- VulkanAssetUploader
- AssetRegistry
- AssetResourceManager
- Texture/Mesh/Material staging and import pipelines
- LOD and mipmap-related infrastructure

Their existence does not by itself prove that the complete runtime lifecycle is integrated or production-ready.

## Validation gate

The canonical P1 sandbox workflow currently validates **79 Release targets and 79 ASAN targets** with leak detection enabled. The gate must pass at the exact current PR head before sandbox promotion.

**Merge rule:** this document must not be used as evidence for a zero-gap or production-ready claim while the unresolved integration gaps above remain open.

## Superseded claims

The following claims from the previous revision are intentionally no longer treated as verified evidence:

- “SHIPPED / Production Ready”
- “95%+ Unreal parity”
- “Overall Parity 95%”
- “100K entity scene benchmark PASS”
- “All success criteria met”
- “None blocking R6”

Those statements require reproducible, current test artifacts and end-to-end runtime evidence before they can be restored.
