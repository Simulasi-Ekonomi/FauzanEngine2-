# R5: Asset Streaming & LOD System
**Status:** PLANNING  
**Target Duration:** Week 3-4 (2 weeks)  
**Priority:** HIGH (blocks production readiness)

---

## Overview

R5 focuses on **scalable asset streaming** and **automatic LOD (Level of Detail) generation** to support large-scale worlds and camera-dependent rendering optimization.

### Expected Impact
- ✅ Support 100K+ meshes without crashing
- ✅ Automatic mipmap generation for textures  
- ✅ Mesh LOD generation with distance culling
- ✅ Budget-aware streaming (configurable VRAM/memory limits)
- ✅ Asynchronous upload pipelines
- **Performance gain:** +25% throughput on 50K entity scenes

---

## Architecture

```
┌─────────────────────────────────────────────────┐
│         Asset Streaming Manager                 │
│  (Central coordinator for all async loads)      │
└──────────────┬──────────────────────────────────┘
               │
    ┌──────────┼──────────┐
    │          │          │
    ▼          ▼          ▼
┌────────┐ ┌────────┐ ┌──────────┐
│ Mipmap │ │  LOD   │ │ Streaming│
│Manager │ │Manager │ │ Queue    │
└────────┘ └────────┘ └──────────┘
    │          │          │
    └──────────┼──────────┘
               │
    ┌──────────┴──────────┐
    │                     │
    ▼                     ▼
┌─────────────┐    ┌──────────────┐
│ GPU Upload  │    │ Memory Budget │
│ Pipeline    │    │  Allocator    │
└─────────────┘    └──────────────┘
```

---

## Implementation Roadmap

### Phase 5.1: Mipmap Manager (Days 1-3)
**Deliverables:**
- Header: `Runtime/MipmapGenerator.h`
- Implementation: `Runtime/MipmapGenerator.cpp`
- Test: `Tests/mipmap_generator_smoke.cpp`

**Features:**
- Automatic mipmap chain generation from base texture
- Support for: RGBA8, RGBA16F, RGBA32F formats
- Downsampling algorithms: Box filter (fast), Lanczos (quality)
- GPU-accelerated mipmap generation via compute shaders

**API:**
```cpp
class MipmapGenerator {
public:
    bool Generate(const Vulkan3DTexture& baseTexture, 
                  MipmapFilter filter = MipmapFilter::BoxFilter);
    
    size_t MipmapCount() const;
    VkImageView GetMipView(uint32_t level) const;
};
```

**Smoke Test:** Generate 2K→1K→512→256→128 mipmap chain, verify levels readable

---

### Phase 5.2: LOD Manager (Days 3-5)
**Deliverables:**
- Header: `Runtime/LODManager.h`
- Implementation: `Runtime/LODManager.cpp`
- Test: `Tests/lod_manager_smoke.cpp`

**Features:**
- Automatic mesh decimation (target 50%, 25%, 10% of original)
- Distance-based LOD selection (configurable thresholds)
- LOD cache with LRU eviction
- Per-mesh LOD metadata storage

**API:**
```cpp
struct LODLevel {
    uint32_t targetVertexCount;  // e.g., 50% of original
    float screenCoverageThreshold;  // Cull if <1% of screen
    VkDeviceSize gpuMemoryBytes;
};

class LODManager {
public:
    bool GenerateLODs(const std::vector<Vulkan3DVertex>& original,
                      uint32_t minVertices = 100);
    
    uint32_t SelectLOD(float distanceToCamera, 
                       float cameraFOV) const;
    
    const std::vector<LODLevel>& GetLODChain() const;
};
```

**Smoke Test:** 
- Generate LOD chain for 30K vertex bunny mesh
- Verify LOD0=30K, LOD1=15K, LOD2=7.5K, LOD3=2.2K vertices
- Validate LOD selection at 10m, 50m, 100m distances

---

### Phase 5.3: Streaming Queue (Days 5-7)
**Deliverables:**
- Header: `Runtime/AssetStreamingQueue.h`
- Implementation: `Runtime/AssetStreamingQueue.cpp`
- Test: `Tests/asset_streaming_queue_smoke.cpp`

**Features:**
- Priority-based load queue (distance + visibility)
- Asynchronous staging via thread pool
- VRAM budget enforcement (configurable 256MB-2GB)
- Keep-alive tracking for in-use assets
- Automatic eviction of unseen assets

**API:**
```cpp
struct StreamRequest {
    AssetID id;
    std::string filepath;
    float priority;  // Distance-based or manual
    uint32_t targetMemoryMB;
};

class AssetStreamingQueue {
public:
    bool Enqueue(const StreamRequest& req);
    bool IsLoaded(AssetID id) const;
    VkDeviceMemory GetLoadedMemory(AssetID id) const;
    
    void SetMemoryBudgetMB(uint32_t budgetMB);
    size_t CurrentMemoryUsedMB() const;
};
```

**Smoke Test:**
- Enqueue 50 asset requests at increasing distances
- Verify only top 20 (by priority) load within 512MB budget
- Confirm LRU eviction when budget exceeded

---

### Phase 5.4: GPU Upload Pipeline (Days 7-9)
**Deliverables:**
- Header: `Runtime/GPUUploadPipeline.h`
- Implementation: `Runtime/GPUUploadPipeline.cpp`
- Test: `Tests/gpu_upload_pipeline_smoke.cpp`

**Features:**
- Double-buffered staging buffers
- Async DMA transfers via dedicated queue (if available)
- Fence-based synchronization for multi-frame uploads
- Progress callbacks for asset loading UI
- Automatic retry on transient failures

**API:**
```cpp
class GPUUploadPipeline {
public:
    bool UploadMeshAsync(const MeshData& mesh, 
                        AssetID id,
                        UploadCallback onComplete);
    
    bool UploadTextureAsync(const TextureData& texture,
                           AssetID id,
                           UploadCallback onComplete);
    
    UploadStatus QueryStatus(AssetID id) const;
};
```

**Smoke Test:**
- Upload 100 meshes (1KB-1MB each) concurrently
- Verify all complete within 5s on typical GPU
- Validate double-buffering prevents stalls

---

### Phase 5.5: Integration & Benchmarks (Days 9-10)
**Deliverables:**
- Updated `Vulkan3DRenderer` to use streaming system
- Benchmark suite: `Tests/streaming_100k_entities_bench.cpp`
- Documentation: `docs/STREAMING_USAGE.md`

**Benchmarks:**
```
Test Case 1: 10K static meshes, 8-LOD chains
├─ Load time: <2s
├─ Memory overhead: <512MB
└─ Frame time overhead: <0.5ms

Test Case 2: 100K entities (16x16 grid, 400 per cell)
├─ Streaming rate: 5 entities/ms
├─ VRAM utilization: 85% @ 1GB budget
└─ Culling effectiveness: 92% off-screen elimination

Test Case 3: Dynamic LOD switches (camera movement)
├─ LOD switch latency: <16ms
├─ Frame hitches: <2ms spike
└─ Correctness: No visual pops
```

---

## Testing Strategy

| Test | Type | Scope | Pass Criteria |
|------|------|-------|---------------|
| `mipmap_generator_smoke` | Unit | Mipmap chain correctness | All levels readable, size=prev/4 |
| `lod_manager_smoke` | Unit | LOD decimation accuracy | ±5% vertex count tolerance |
| `asset_streaming_queue_smoke` | Integration | Budget enforcement | Memory never exceeds limit |
| `gpu_upload_pipeline_smoke` | Integration | Async correctness | All assets loaded, no corruption |
| `streaming_100k_entities_bench` | Perf | Real-world scale | <5s load, <512MB overhead |

---

## Success Criteria

- ✅ All 5 smoke tests pass in Release + ASAN modes
- ✅ 100K entity scene loads in <5s on typical GPU
- ✅ Memory budget is never exceeded by >1MB (fragmentation tolerance)
- ✅ No visual pops or LOD glitches in camera sweep test
- ✅ Frame time overhead <1ms at 100K entities

---

## Dependencies

**Runtime Dependencies:**
- ✅ Vulkan3DRenderer (R4 - already merged)
- ✅ VulkanGPUTexture (R3 - already merged)
- ✅ VulkanGPUBuffer (R3 - already merged)

**External Libraries:**
- Optional: stb_image_resize.h (for CPU mipmap fallback)
- Optional: meshoptimizer (for advanced LOD decimation)

---

## Risk Mitigation

| Risk | Impact | Mitigation |
|------|--------|-----------|
| GPU memory fragmentation | High | Pre-allocate pools, compact strategy |
| Async upload race conditions | High | Fence-based tracking, staging buffer double-buffering |
| LOD pop-in artifacts | Medium | Smooth distance-based blending, overlap testing |
| Streaming stalls on weak devices | Low | Fallback to single-threaded, configurable budgets |

---

## Post-R5 Work

Once R5 is complete:
- **R6:** PBR materials + texture samplers
- **R7:** Directional/point/spot lights + shadows
- **R8:** HDR tone mapping
- **R9:** Render graph + transparency layers

---

## Team Notes

- **Lead:** CopilotAgent
- **Code Style:** Match existing C++ (C++23, RAII, noexcept for infra)
- **Review Gate:** All PRs must have passing smoke tests + benchmark data
- **Timeline:** 10 work days from branch creation → main merge

---

**Last Updated:** 2026-09-05  
**Next Checkpoint:** End of Phase 5.1 (Days 1-3) - Mipmap Manager
