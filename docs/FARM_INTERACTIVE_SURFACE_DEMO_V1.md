# Farm Interactive Surface Demo V1

`RunFarmInteractiveSurfaceDemo` is a finite CPU/software proof harness for the interactive Farm slice. It initializes an isolated 4×4 Farm runtime, moves the character one tile right through ordinary canonical input, then renders and presents nineteen hidden-surface frames. It selects TILL, PLANT, WATER, and HARVEST through retained HUD pointer paths and executes each only on a later canonical `farm_interact` input at the moved character tile. Crop growth advances through ordinary one-tick Farm frames; no special UI time path exists.

The final composed software frame is written to a caller-supplied P6 PPM path. The receipt commits only after all asset setup, movement, session frames, HUD draws, hidden presentation, all four action selections, world changes, and PPM write succeed. It records world and HUD hashes separately, frame/presentation counts, final character position, a four-action selection mask, final selected action, Farm telemetry, and wheat inventory snapshot.

This is a finite test/demo harness, not a persistent game loop or game host. It provides no direct UI world authority, project filesystem pipeline beyond the explicit PPM artifact, save service, networking, advertising/monetization, APK, or production release behavior.
