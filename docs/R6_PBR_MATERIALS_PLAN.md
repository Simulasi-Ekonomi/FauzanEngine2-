# R6: PBR Materials & Dynamic Lighting
**Status:** PLANNING  
**Target Duration:** Week 5-6 (2 weeks)  
**Priority:** HIGH (visual fidelity baseline)  
**Dependency:** ✅ R5 (Asset Streaming complete)

---

## Overview

R6 focuses on **physically-based rendering (PBR) materials** and **dynamic lighting system** to bring visual fidelity to production quality. This phase establishes the foundation for advanced rendering techniques and enables realistic material representation.

### Expected Impact
- ✅ Support industry-standard PBR workflow (Metallic/Roughness)
- ✅ Dynamic point/directional/spot lights with real-time shadows
- ✅ Material parameter streaming via asset pipeline
- ✅ Deferred rendering pipeline for multi-light scenes
- ✅ Shadow mapping (cascade + omnidirectional)
- **Visual Impact:** Production-ready material appearance + dynamic lighting
- **Performance Target:** 60 FPS with 32+ dynamic lights @ 1080p

---

## Architecture

```
┌────────────────────────────────────────────────────┐
│         PBR Material System                        │
│  (Unified shader + material parameter management) │
└──────────────┬───────────────────────────────────┘
               │
    ┌──────────┼──────────┐
    │          │          │
    ▼          ▼          ▼
┌─────────┐ ┌────────┐ ┌──────────┐
│ Material│ │Shader  │ │Texture   │
│Compiler │ │Library │ │Sampler   │
└─────────┘ └────────┘ └──────────┘
    │          │          │
    └──────────┼──────────┘
               │
    ┌──────────┴──────────┐
    │                     │
    ▼                     ▼
┌──────────────┐    ┌────────────────┐
│ Deferred G-  │    │ Light Culling  │
│ Buffer Pass  │    │ + Composition  │
└──────────────┘    └────────────────┘
    │                     │
    └──────────┬──────────┘
               │
    ┌──────────┴──────────┐
    │                     │
    ▼                     ▼
┌──────────────┐    ┌────────────────┐
│Shadow Map    │    │Forward Pass    │
│Generation    │    │(Transparent)   │
└──────────────┘    └────────────────┘
```

---

## Implementation Roadmap

### Phase 6.1: Material System Foundation (Days 1-2)
**Deliverables:**
- Header: `Runtime/PBRMaterial.h`
- Implementation: `Runtime/PBRMaterial.cpp`
- Shader: `Shaders/pbr_material.frag`
- Test: `Tests/pbr_material_smoke.cpp`

**Features:**
- Metallic/Roughness PBR model
- Support for: BaseColor, Metallic, Roughness, Normal, AO maps
- Material parameter serialization (save/load)
- BRDF lookup table (LUT) generation
- Material caching with LRU eviction

**API:**
```cpp
struct PBRMaterialParams {
    glm::vec3 baseColor = {0.8f, 0.8f, 0.8f};
    float metallic = 0.0f;           // [0, 1]
    float roughness = 0.5f;          // [0, 1]
    float normalMapStrength = 1.0f;
    float aoStrength = 1.0f;
    glm::uint32_t flags = 0;
};

class PBRMaterial {
public:
    bool Load(const std::string& filepath) noexcept;
    bool SetTexture(TextureSlot slot, VkImageView imageView) noexcept;
    
    const PBRMaterialParams& GetParams() const noexcept;
    void SetParams(const PBRMaterialParams& params) noexcept;
    
    VkDescriptorSet GetDescriptorSet() const noexcept;
};
```

**Smoke Test:** 
- Load PBR material from file (JSON + textures)
- Verify all texture slots bound correctly
- Validate BRDF LUT generated correctly

---

### Phase 6.2: Shader Library & Sampler System (Days 2-4)
**Deliverables:**
- Header: `Runtime/ShaderLibrary.h`
- Implementation: `Runtime/ShaderLibrary.cpp`
- Shaders: `Shaders/pbr_vertex.vert`, `pbr_lighting.frag`
- Test: `Tests/shader_library_smoke.cpp`

**Features:**
- Centralized shader compilation + caching
- Shader variant generation (e.g., with/without normal map)
- Sampler state management (anisotropic filtering, comparison, etc.)
- Shader hot-reload on development mode
- Preprocessor directive support (#define for features)

**API:**
```cpp
enum class TextureSampler : uint8_t {
    LinearRepeat,
    LinearClamp,
    NearestRepeat,
    ComparisonDepth,
    // ...
};

class ShaderLibrary {
public:
    [[nodiscard]] bool CompileShader(const std::string& name, 
                                     const std::filesystem::path& filepath) noexcept;
    
    [[nodiscard]] VkPipelineLayout GetPipelineLayout(const std::string& shader) const noexcept;
    [[nodiscard]] VkShaderModule GetShaderModule(const std::string& shader, 
                                                 VkShaderStageFlagBits stage) const noexcept;
    
    [[nodiscard]] VkSampler GetSampler(TextureSampler type) const noexcept;
};
```

**Smoke Test:**
- Compile 3 shader variants (with/without normal map, with displacement)
- Verify all modules load correctly
- Test sampler creation for all types

---

### Phase 6.3: Deferred Rendering Pipeline (Days 4-6)
**Deliverables:**
- Header: `Runtime/DeferredRenderer.h`
- Implementation: `Runtime/DeferredRenderer.cpp`
- Shaders: `Shaders/gbuffer_pass.frag`, `lighting_compose.frag`
- Test: `Tests/deferred_renderer_smoke.cpp`

**Features:**
- G-Buffer generation (Position, Normal, Albedo+Metallic, Roughness+AO)
- Lighting composition pass
- Light culling via compute shader
- Light list generation per tile (16x16 tiles recommended)
- Support for up to 256 dynamic lights per frame

**G-Buffer Layout:**
```glsl
// GBuffer0: Position (RGB32F)
// GBuffer1: Normal (RG16F) + Metallic (R16F) + Roughness (G16F)
// GBuffer2: Albedo (RGB8) + AO (A8)
// Depth: Standard depth buffer (D32F)
```

**API:**
```cpp
class DeferredRenderer {
public:
    [[nodiscard]] bool Initialize(uint32_t width, uint32_t height) noexcept;
    
    // G-Buffer generation
    [[nodiscard]] bool RenderGBuffer(const std::vector<RenderCommand>& commands) noexcept;
    
    // Light composition
    [[nodiscard]] bool ComposeLighting(const std::vector<Light>& lights,
                                       VkImageView outputTarget) noexcept;
    
    [[nodiscard]] VkImageView GetGBufferView(uint32_t index) const noexcept;
};
```

**Smoke Test:**
- Render G-Buffer from 100 opaque objects
- Compose lighting with 16 point lights
- Verify output image is non-black/non-uniform

---

### Phase 6.4: Dynamic Lighting System (Days 6-8)
**Deliverables:**
- Header: `Runtime/DynamicLight.h`, `Runtime/LightingManager.h`
- Implementation: `Runtime/DynamicLight.cpp`, `Runtime/LightingManager.cpp`
- Shaders: `Shaders/shadow_map.vert`, `shadow_map.frag`
- Test: `Tests/dynamic_lighting_smoke.cpp`

**Features:**
- Point light (omnidirectional shadows via cube map)
- Directional light (cascade shadow mapping - 4 cascades)
- Spot light (2D shadow mapping + cone angle)
- Shadow map caching + reuse
- Inverse square law falloff
- Light attenuation visualization

**Light Structures:**
```cpp
struct PointLight {
    glm::vec3 position;
    float radius;                 // Attenuation radius
    glm::vec3 color;
    float intensity;              // Linear intensity
    VkImageView shadowCubeMap = VK_NULL_HANDLE;
};

struct DirectionalLight {
    glm::vec3 direction;          // Normalized
    float shadowBias = 0.005f;
    glm::vec3 color;
    float intensity;
    std::array<VkImageView, 4> cascadeShadowMaps;  // CSM
};

struct SpotLight {
    glm::vec3 position;
    float innerConeAngle;         // Radians
    glm::vec3 direction;
    float outerConeAngle;         // Radians
    glm::vec3 color;
    float intensity;
    float radius;
    VkImageView shadowMap = VK_NULL_HANDLE;
};
```

**API:**
```cpp
class LightingManager {
public:
    [[nodiscard]] uint32_t AddPointLight(const PointLight& light) noexcept;
    [[nodiscard]] uint32_t AddDirectionalLight(const DirectionalLight& light) noexcept;
    [[nodiscard]] uint32_t AddSpotLight(const SpotLight& light) noexcept;
    
    bool UpdateLight(uint32_t lightID, const PointLight& light) noexcept;
    bool RemoveLight(uint32_t lightID) noexcept;
    
    [[nodiscard]] const std::vector<PointLight>& GetPointLights() const noexcept;
    [[nodiscard]] uint32_t GetActiveLightCount() const noexcept;
};
```

**Smoke Test:**
- Add 8 point lights to scene
- Add 1 directional light with CSM
- Add 4 spot lights
- Verify shadow maps generated correctly
- Check light list SSBO populated

---

### Phase 6.5: Integration & Benchmarks (Days 8-10)
**Deliverables:**
- Updated `Vulkan3DRenderer` to use deferred pipeline
- Benchmark suite: `Tests/pbr_lighting_bench.cpp`
- Documentation: `docs/MATERIAL_USAGE.md`, `docs/LIGHTING_SETUP.md`
- Example scene: `Assets/demo_pbr_materials.scene`

**Benchmarks:**
```
Test Case 1: Static scene, 50K meshes, 8 PBR materials
├─ G-Buffer pass: <3ms @ 1080p
├─ Lighting compose: <2ms (8 point lights)
└─ Frame time: ~60 FPS

Test Case 2: Dynamic lights, 10K meshes, 32 point lights
├─ Shadow map updates: <5ms
├─ Lighting composition: <4ms
└─ Frame time: 60 FPS (GPU bound)

Test Case 3: CSM with 4 cascades, 1 directional light
├─ Cascade generation: <2ms
├─ Shadow sampling overhead: <1ms
└─ Visual quality: Sharp shadows at all distances
```

---

## Material Workflow

### Creating a PBR Material (Step-by-step)

1. **Export from Substance/Marmoset:**
   ```
   Material/Metal_Plate/
   ├─ BaseColor.png (SRGB)
   ├─ Normal.png (Linear)
   ├─ Metallic.png (Linear, R-channel only)
   ├─ Roughness.png (Linear, R-channel only)
   └─ AO.png (Linear, R-channel only)
   ```

2. **Create material definition:**
   ```json
   {
     "name": "Metal_Plate",
     "baseColorTexture": "BaseColor.png",
     "normalTexture": "Normal.png",
     "metallicTexture": "Metallic.png",
     "roughnessTexture": "Roughness.png",
     "aoTexture": "AO.png",
     "parameters": {
       "normalMapStrength": 1.0,
       "aoStrength": 1.0
     }
   }
   ```

3. **Load in runtime:**
   ```cpp
   PBRMaterial material;
   material.Load("Materials/Metal_Plate.material");
   ```

4. **Apply to mesh:**
   ```cpp
   renderer->BindMaterial(meshID, material.GetDescriptorSet());
   ```

---

## Testing Strategy

| Test | Type | Scope | Pass Criteria |
|------|------|-------|---------------|
| `pbr_material_smoke` | Unit | Material loading & params | All textures bound, params valid |
| `shader_library_smoke` | Unit | Shader compilation & caching | All variants compile, samplers created |
| `deferred_renderer_smoke` | Integration | G-Buffer generation | All G-Buffer targets populated correctly |
| `dynamic_lighting_smoke` | Integration | Light system + shadows | Shadow maps generated, lights culled correctly |
| `pbr_lighting_bench` | Perf | Real-world scene | 60 FPS @ 1080p with 32 lights |

---

## Success Criteria

- ✅ All 5 smoke tests pass in Release + ASAN modes
- ✅ PBR materials visually match Substance Player reference
- ✅ 60 FPS maintained with 32 dynamic point lights @ 1080p
- ✅ Shadow quality: Aliasing < 1 pixel @ 1m distance
- ✅ No visible light popping when lights added/removed
- ✅ Material hot-reload works without crashes
- ✅ CSM cascades blend seamlessly (no seams visible)

---

## Dependencies

**Runtime Dependencies:**
- ✅ Vulkan3DRenderer (R4 - already available)
- ✅ AssetStreamingQueue (R5 - materials load via streaming)
- ✅ ShaderLibrary (new - built this phase)
- ✅ VulkanGPUTexture (R3 - texture binding)

**External Libraries:**
- glm (math library - already used)
- VulkanMemoryAllocator (already used)

**SPIR-V Compilation:**
- glslc or glslang for offline shader compilation

---

## Risk Mitigation

| Risk | Impact | Mitigation |
|------|--------|-----------|
| Shader compilation slow | Medium | Pre-compile offline, cache SPIR-V |
| Light culling overhead | Medium | Use compute shader + tile-based binning |
| Shadow map aliasing | Medium | Use PCF or Poisson sampling |
| Memory for 32 light shadows | High | Reuse shadow maps, cache strategy |
| Deferred + transparent overlap | Medium | Forward pass for transparent objects |

---

## Performance Targets

### Deferred Pipeline
- **G-Buffer pass:** < 3ms @ 1080p
- **Lighting composition:** < 2ms (8 lights), < 4ms (32 lights)
- **Shadow map generation:** < 5ms (8 lights)
- **Total overhead:** < 10ms for 32 lights

### Material System
- **Material loading:** < 100ms (including texture streaming)
- **Shader compilation:** < 500ms (offline), < 50ms (cached)
- **Parameter update:** < 0.1ms

---

## Post-R6 Work

Once R6 is complete:
- **R7:** Advanced shadows (PCF/PCSS) + ambient occlusion
- **R8:** HDR + tone mapping + bloom
- **R9:** Render graph refactoring + transparency ordering
- **R10:** Post-processing pipeline (FXAA, color grading)

---

## Team Notes

- **Lead:** CopilotAgent
- **Code Style:** Match existing C++ (C++23, RAII, noexcept for infra)
- **Shader Style:** GLSL 450+ with appropriate qualifiers (coherent, volatile, restrict)
- **Review Gate:** All PRs must have passing smoke tests + benchmark data
- **Timeline:** 10 work days from branch creation → main merge
- **Parallel work:** Can start R6 while R5 benchmarks run

---

**Created:** 2026-09-05  
**Kickoff:** Ready after R5 merged (main: b6552a5...)  
**Next Checkpoint:** End of Phase 6.1 (Days 1-2) - Material System
