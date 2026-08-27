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
7. The result preserves every completed stage while explicitly classifying acquire, submit, fence-wait, and present driver results as device lost, surface out-of-date, timeout, or other driver rejection; only success/suboptimal results may reach `Presented`.

Observed Release output:

```text
VULKAN_TEXTURED_PRESENT_SMOKE_OK surface=1 swapchain=1 upload=1 pipeline=1 submit=1 present=1 status=7 images=4 staged=1 hash=1142008413756203960
VULKAN_TEXTURED_PRESENT_SMOKE_OK surface=1 swapchain=1 upload=1 pipeline=1 submit=1 present=1 status=7 images=4 staged=1 hash=1142008413756203960
TEXTURED_PRESENT_XVFB_SMOKE_OK repeat=2
```

## Boundary

`vulkan_present_status_smoke` validates the pure status classifier in Release and AddressSanitizer with `detect_leaks=1`; it does not create SDL, X11, or a Vulkan device. The full Xvfb textured-present smoke passes in Release. In this sandbox, the full Xvfb AddressSanitizer execution reports a 224-byte indirect leak from external X11/driver worker libraries after engine teardown, so it is deliberately **not** claimed as passing and remains an environmental/teardown investigation blocker.

The status classifier is tested with Vulkan result values directly; it is **not** a forced physical device-loss or resize experiment. This proves a textured GPU swapchain present path under a virtual X11 display plus a fail-closed driver-outcome contract. It does not yet prove integration with `NeoRuntime` Farm scene ownership, camera/material authoring, mesh scene rendering, resize/recreation, device-loss recovery, Android presentation, accessibility, or a shippable player client.
