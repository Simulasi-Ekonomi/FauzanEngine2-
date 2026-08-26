# Farm Interactive Surface Demo V1

`RunFarmInteractiveSurfaceDemo` is a finite CPU/software proof harness for the interactive Farm slice. It initializes an isolated 4×4 Farm runtime, renders and presents four hidden-surface frames, performs TILL through the default canonical input bridge action, selects PLANT through the retained HUD pointer path, then performs PLANT only by a later canonical `farm_interact` input.

The final composed software frame is written to a caller-supplied P6 PPM path. The receipt commits only after all asset setup, session frames, HUD draws, hidden presentation, action selection, world changes, and PPM write succeed. It records world and HUD hashes separately, frame/presentation counts, selected action, Farm telemetry, and wheat inventory snapshot.

This is a finite test/demo harness, not a persistent game loop or game host. It provides no direct UI world authority, project filesystem pipeline beyond the explicit PPM artifact, save service, networking, advertising/monetization, APK, or production release behavior.

