# P1 Unreal-like parity sandbox

This sandbox is an evidence gate, not a percentage generator.

It performs four checks:

1. Configures the canonical `Source/NeoEngine` P1 runtime with CMake.
2. Builds the canonical runtime.
3. Discovers every executable registered through `add_xpbd_executable`, builds each target, and executes each smoke with a timeout.
4. Produces separate static marker evidence for TODO/FIXME/placeholder/not-implemented/stub patterns.

A PASS means the target executable built and exited successfully. It does **not** by itself prove Unreal Engine parity.

The final classification must distinguish:
- functional runtime code,
- compile-only code,
- smoke-tested code,
- integration-tested code,
- placeholder/stub/minimal code,
- missing production integration.

The sandbox intentionally does not delete, replace, or downgrade P1 code.
