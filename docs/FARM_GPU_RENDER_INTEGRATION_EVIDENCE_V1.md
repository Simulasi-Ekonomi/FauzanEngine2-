# Farm GPU Render Integration Evidence V1

## Scope

Increment ini menyambungkan output frame dari `NeoRuntime::RenderFarm` ke `VulkanTexturedPresentProbe` melalui overload pixel-span. Frame berasal dari `SoftwareRenderer` canonical setelah NeoRuntime tick dan Farm HUD composition; probe kemudian menjalankan hidden SDL window, Vulkan surface/device, FIFO swapchain, staged texture upload, existing `neo_texture` shader pipeline, submit, dan present.

## Evidence

`farm_vulkan_textured_present_smoke` membuktikan:

| Capability | Evidence |
|---|---|
| Canonical frame source | NeoRuntime initialized and ticked; `RenderFarm` produced a non-zero Farm/HUD framebuffer hash and pixel span. |
| Invalid framebuffer handling | Pixel span length mismatch is rejected as `VulkanPresentStatus::InvalidInput` without presenting a frame. |
| Texture upload | Renderer pixels are converted to bounded RGBA texture bytes and uploaded through the existing textured-present path. |
| Swapchain presentation | Release and ASAN/Xvfb report created window/surface/device/swapchain, uploaded texture, created pipeline, submitted frame, and presented frame. |
| Farm/HUD composition | The source frame includes canonical Farm runtime/HUD composition and curriculum receipt state before Vulkan presentation. |

Release and ASAN/Xvfb receipt:

```text
FARM_VULKAN_TEXTURED_PRESENT_SMOKE_OK farm_hash=16382323198459797299 texture_hash=2010794924154051204 swapchain=4 upload=1 submit=1 present=1 lessons=0
```

Workflow `.github/workflows/r3-farm-render.yml` reproduces both configurations on Ubuntu with Vulkan/SDL2/Xvfb dependencies.

## Boundary

This closes a bounded Farm framebuffer-to-Vulkan textured-present proof. It does not prove a production GPU scene renderer, mesh/material/animation integration, lighting/camera parity, physical-GPU coverage, device-loss recovery, resize/recreate, asset streaming, performance budget, or Android GPU delivery. ASAN uses `detect_leaks=0` because the existing virtual Vulkan/X11 driver boundary is external to engine ownership.
