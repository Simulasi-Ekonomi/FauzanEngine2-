# Farm Runtime Frame Receipt V1

`FarmRuntimeSession` now retains a small `FarmRuntimeFrameReceipt`: the committed frame number and software framebuffer hash. The receipt begins as zero on successful initialization and is updated only after input bridging, world tick, and software render have all succeeded.

Invalid frame ticks, rejected input, rejected world ticks, or rejected rendering leave the prior receipt untouched. The receipt owns no simulation, asset registry, input state, gameplay authority, persistence, host loop, or external effect. The Farm runtime smoke proves matching receipt/hash after a committed frame and receipt preservation on invalid tick and rejected input. It passes in Release and AddressSanitizer with `detect_leaks=1`; current non-Vulkan broad suites pass 128/128 in both configurations. This is finite in-process runtime evidence, not a persistent game host, networking, save system, APK, or release readiness.
