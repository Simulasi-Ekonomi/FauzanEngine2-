# NeoRuntime Frame Receipt V1

`NeoRuntimeFrameReceipt` is an immutable caller-visible copy committed only after one `NeoRuntime::Tick()` completes clock advance, deterministic game-time advance, optional movement, actor fixed tick, Farm/world tick and scene sync, authoring tick, and event dispatch.

The receipt copies clock and game-time snapshots, actor fixed-tick counters, Farm telemetry, Farm-world snapshot, and the number of events accepted for dispatch. `RenderFarm()` adds a copy of its existing `RuntimeFarmRenderReceipt` only after world rendering, optional HUD, and optional surface presentation have succeeded. The clock snapshot remains the existing frame-clock contract; its `fixedStepCount` is not repurposed to mirror `RuntimeTimeSnapshot::hostFixedStepCount`. A paused or zero-simulated-tick frame may still commit a receipt after dispatch; its actor receipt is empty because no actor tick ran.

If a phase fails, the runtime reports its existing error and leaves the prior frame receipt intact. The receipt is reset at initialization and shutdown. The runtime smoke injects an actor fixed-tick rejection and verifies that this preservation occurs. It is orchestration metadata only: it does not create a second simulation loop, permit UI or client authority, own audio-device output, persist data, or imply networking, monetization, Android, or production readiness.
