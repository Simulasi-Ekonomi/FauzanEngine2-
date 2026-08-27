# Vulkan Textured Swapchain Present Evidence V1

## Scope

The canonical runtime now has a bounded textured Vulkan present probe. It creates a hidden SDL Vulkan window and surface, selects a graphics queue that supports presentation, creates a FIFO swapchain, uploads caller-provided RGBA bytes into a device-local sampled image, binds the image through a combined image sampler descriptor, draws the existing `neo_texture` shader pipeline into a swapchain image, submits with an acquire/render-finished semaphore pair, and presents under Xvfb.

## Acceptance evidence

The canonical `vulkan_textured_present_smoke` performs the following checks:

1. A zero-sized output is rejected without creating a window.
2. A bounded 2×2 RGBA texture is accepted.
3. SDL window and Vulkan surface creation succeed.
4. A present-capable device and swapchain are created.
5. Texture upload, sampler, descriptor set, shader pipeline, command recording, queue submission, and presentation all report success.
6. The input texture content hash is deterministic across two independent Xvfb executions.

Observed Release output:

```text
VULKAN_TEXTURED_PRESENT_SMOKE_OK surface=1 swapchain=1 upload=1 pipeline=1 submit=1 present=1 images=4 hash=1142008413756203960
VULKAN_TEXTURED_PRESENT_SMOKE_OK surface=1 swapchain=1 upload=1 pipeline=1 submit=1 present=1 images=4 hash=1142008413756203960
TEXTURED_PRESENT_XVFB_SMOKE_OK repeat=2
```

## Boundary

This proves a textured GPU swapchain present path under a virtual X11 display. It does not yet prove integration with `NeoRuntime` Farm scene ownership, camera/material authoring, mesh scene rendering, resize/recreation, device-loss recovery, Android presentation, accessibility, or a shippable player client.
