# NeoRuntime Farm Player Input V1

`RuntimeConfig::enableFarmPlayerInput` is default-off. When enabled, `NeoRuntime` creates one `InputState`, binds five named Farm actions to its own keyboard code range, initializes one `FarmPlayerInputBridge`, and executes that bridge after actor fixed tick but before the existing `FarmWorldTool::Tick`.

The bridge continues to be the sole interpreter of the five Farm action names. It rejects missing bindings, conflicting movement plus interact, invalid coordinates, and rejected world actions. `NeoRuntime` commits a `FarmPlayerInputReceipt` into `NeoRuntimeFrameReceipt` only after all existing tick phases complete. A bridge failure moves the runtime to `Failed` and leaves the prior receipt intact, following the candidate-commit contract.

This is not a UI-to-world connection: UI selection must remain indirect through the existing action-panel-to-bridge route. The increment does not create a `FarmRuntimeSession` inside `NeoRuntime`, a second simulation loop, save service, network authority, audio device, Android packaging, monetization, or production claim.
