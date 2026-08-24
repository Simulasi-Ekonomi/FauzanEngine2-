# Renderer Capability Status

The canonical CMake runtime does not link a validated graphics backend. `RendererCapabilityProbe` therefore returns `NotImplemented`, backend `none`, and an explicit reason. It prevents future runtime callers from treating legacy renderer source files as an operational graphical path.

```text
RENDERER_CAPABILITY_SMOKE_OK state=not_implemented
```

Release and AddressSanitizer validation pass for the probe. Farm and Sudoku can run as authoritative terminal/state runtimes, but their graphical presentation remains blocked until a renderer backend is selected, built into the canonical runtime, and validated with a real window/surface, mesh/material resource lifetime, camera, lighting, input, and frame presentation loop.
