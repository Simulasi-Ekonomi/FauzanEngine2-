# Editor Tooling V1 Browser Evidence

Snapshot verified locally at `http://localhost:4173/` after the Editor V1 checkpoint build.

The rendered surface visibly contains the NEOENGINE menu bar, File/Edit/Window/Tools/Build/Help, save/undo/redo toolbar, transform mode controls, World/Local space, snap/grid controls, actor creation controls, Play control, World Outliner, Viewport, Content Browser, Aries AI Console, Details inspector, and status bar.

Browser interaction smoke: clicking the Add Cube toolbar control increased the Outliner/Viewport actor count from 4 to 5 after the next render. The new Cube was selected, Details fields appeared, the toolbar showed the dirty marker, and the status bar showed `Unsaved` plus `Selected: Cube`. The viewport rendered the selected cube with a visible transform gizmo. This proves the frontend build is not only static markup; scene mutation and selection state propagate through the UI.

The initial immediate DOM read occurred before React's next render and reported the old actor count; a subsequent browser view confirmed the committed state. This timing note is retained as evidence that the acceptance check waits for the render boundary.

Second browser smoke: selecting the Cube kept Details visible; editing the Location X field to `2.5` moved the rendered cube to the right in the viewport, updated its inspector value, and retained the `Unsaved` dirty marker. The transform gizmo remained visible on the selected actor.

Third browser smoke: clicking `+ Add Component` on the selected Cube added a `Scene (SceneComponent)` section with an editable `enabled` checkbox in Details. The scene remained marked `Unsaved`, confirming component mutation participates in the editor dirty workflow.

Fourth browser smoke: the Parent selector listed all other actors and changing Cube's parent to DirectionalLight updated the Outliner into a nested tree (`DirectionalLight` → `Cube`) while preserving selection, inspector, dirty state, and viewport rendering.
