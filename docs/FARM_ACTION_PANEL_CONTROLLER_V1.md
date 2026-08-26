# Farm Action Panel Controller V1

`FarmActionPanelController` binds exactly four distinct UI widget IDs to the existing canonical `FarmPlayerAction` values: Till, PlantWheat, Water, and Harvest. Its only external effect is calling `FarmPlayerInputBridge::SetSelectedAction` after a validated panel activation.

Pointer selection is committed only on a routed release of a configured panel widget. Keyboard selection is committed only on a routed `Activate` event for a configured focused widget. Pointer/keyboard events for other interactive UI widgets are intentionally ignored, enabling future HUD controls without unintentionally changing the selected Farm action.

Initialization requires one unique binding for every supported action. Duplicate widget IDs, duplicate actions, unknown widget selection, non-ready input bridges, and invalid router keys fail closed and preserve the prior bridge action and caller receipt where applicable.

The controller never calls `FarmWorldTool`, does not perform Till/Plant/Water/Harvest itself, and does not tick `FarmRuntimeSession`. Existing interact input remains the sole path that performs the later world action. This is CPU/in-memory UI selection only; it provides no persistence, network authority, advertising, monetization, APK, or production UI behavior.

