# NeoRuntime Farm Vertical Slice V1

## Scope

This smoke composes existing `NeoRuntime` ownership only. It initializes the runtime with its Farm HUD, `FarmPlayerInputBridge`, CPU renderer, and hidden `SoftwareSurfacePresenter`; drives keyboard focus/activation into the HUD; then sends the canonical interact input on a later runtime tick. It does not construct or tick `FarmRuntimeSession`.

The bounded script carries one tile through Till, Plant Wheat, Water, growth ticks, and Harvest. It verifies the HUD overlay differs from the world framebuffer, the software presenter receives the committed CPU framebuffer, and inventory changes to 31 wheat seeds and 2 wheat produce. It then saves a topology-preserving NeoRuntime Farm progress checkpoint, moves the player through the canonical input bridge, restores the checkpoint, and verifies the world and render receipts recover/fail closed as contracted.

## Evidence and boundary

`neo_runtime_farm_vertical_slice_smoke` passes in Release and AddressSanitizer with `ASAN_OPTIONS=detect_leaks=1` using the SDL dummy video driver. Its corrupt-checkpoint case keeps the Farm world and last committed render receipt intact.

This is a deterministic, finite host smoke over a hidden CPU presentation surface. It is not interactive user acceptance evidence, an Android or GPU surface test, a persistent save system, online multiplayer, payment flow, anti-cheat proof, accessibility evaluation, or a shippable game-client claim.
