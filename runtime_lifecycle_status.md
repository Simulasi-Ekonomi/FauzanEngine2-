# Canonical Runtime Lifecycle Status

`NeoRuntime` is the active bounded lifecycle entry point for the recovered runtime core. It has four explicit states: `Created`, `Initialized`, `Shutdown`, and `Failed`. Initialization validates configuration and creates the authoritative Farm subsystem. Tick runs only from `Initialized`; illegal transitions fail explicitly. Shutdown releases owned runtime state and prohibits further ticks.

```text
RUNTIME_SMOKE_OK lifecycle=init_tick_shutdown
```

The smoke scenario verifies a rejected pre-init tick, 1,000-tile Farm initialization, tick-driven crop growth and harvest, orderly shutdown, and rejected post-shutdown tick. Release and AddressSanitizer validation pass.

`NeoRuntime` now also owns the canonical `AssetRegistry` for its initialized lifetime. The extended smoke test declares and readies a Farm texture metadata entry, then confirms registry ownership is released at shutdown. This remains metadata-only until an importer and validated renderer upload path are available.

This lifecycle does **not** claim a graphical renderer. It is the canonical game-state execution loop to which a validated renderer, Android activity bridge, and presentation UI must attach later. The legacy `Core/EngineLoop` remains outside the active CMake source set and must not be used as evidence of a production renderer lifecycle.
