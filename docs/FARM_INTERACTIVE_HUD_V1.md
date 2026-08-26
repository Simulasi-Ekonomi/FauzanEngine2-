# Farm Interactive HUD V1

`FarmRuntimeHud` retains a layout-resolved UI router after its first enhanced draw on a software surface of at least 128×96 pixels. The enhanced overlay draws frame, coins, tick, growing/harvestable counts, and canonical wheat-produce inventory alongside four focusable action widgets: TILL, PLANT, WATER, and HARVEST. Smaller 64×48-compatible surfaces retain the compact read-only overlay and intentionally expose no action input.

`FarmRuntimeSession::RouteHudPointer` and `RouteHudKeyboard` delegate only to `FarmActionPanelController`, which changes the session's existing `FarmPlayerInputBridge` selected action. The UI never calls `FarmWorldTool`. A later `FarmRuntimeSession::Frame` with canonical `farm_interact` input remains the only path that executes Till, Plant, Water, or Harvest against the Farm world. The focused proof selects all four retained action widgets and advances growth solely through ordinary Farm frames before harvesting.

The keyboard path uses the router's existing focus traversal and `Activate` event. An invalid keyboard enum is rejected by the router/controller/session chain and preserves the previously selected Farm action plus caller action receipt. No keyboard binding persistence or remapping is introduced.

The session commits wheat seed/produce counts into immutable frame and HUD receipts from `FarmSystem::ItemCount` after a successful world tick and render. Invalid or unframed HUD input fails closed and preserves the selected action and caller receipt. HUD composition uses a candidate software renderer and has no persistence, network authority, advertising/monetization, APK, or production game claim.
