# UI Layout Resolver V1

`UiLayoutResolver` is a bounded CPU/software geometry resolver for widgets already retained by `UiInputRouter`. It resolves one non-empty, ordered plan against an explicit finite surface and commits resulting rectangles only when the entire candidate plan succeeds.

Each layout spec identifies an existing widget. A child spec must appear after its parent spec, making parent-relative geometry and vertical sibling order deterministic. The resolver supports four fixed-size corner anchors, fill sizing, non-negative per-widget padding, fixed or fill width where applicable, positive minimum dimensions, and vertical stacks. `UiCanvasRenderer` and `UiInputRouter` continue to consume the resulting `UiWidgetSpec::rect` values without any input-dispatch change.

Invalid surfaces, duplicate/missing widgets, unresolved parents, non-finite or invalid dimensions, and insufficient parent space fail closed. The router's prior geometry, focus, capture, and canvas-facing state remain intact because resolution occurs on a candidate router copy.

This contract is limited to retained UI geometry in the CPU/software runtime. It does not provide Farm commands, widget images, persistence, window ownership, GPU UI, accessibility, monetization, APK packaging, or production UI behavior.

