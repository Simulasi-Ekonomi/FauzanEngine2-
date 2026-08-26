# UI Canvas Image V1

`UiCanvasRenderer::SetImage` binds one visible UI widget to one current `CpuTextureResource` already owned by `TextureStagingStore` and verified against its ready `AssetRegistry` definition. The binding stores the approved content hash, optionally accepts a bounded source rectangle, and rejects duplicate widget images, malformed decoded resources, stale registry state, or invalid source rectangles.

`Draw` revalidates every image binding before composing any pixels. It then draws every panel, image, and label into one candidate `SoftwareRenderer`; the caller renderer changes only after the complete canvas succeeds. Therefore a staged texture that becomes stale, disappears, or no longer matches the approved registry hash fails closed without partially drawing the canvas.

This is a bounded CPU/software image widget. It does not import images, watch files, upload to GPU, own asset lifetime, provide animation, change Farm/world authority, persist UI state, monetize, package an APK, or claim production UI readiness.

