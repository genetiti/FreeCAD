# Open Research Questions

Questions to resolve during phase planning/research. Each should be verified against **version-pinned** official SOLIDWORKS Help before implementation, since some are version-introduced features.

## SolidWorks UI parity (from deep-research, 2026-06-06)

These were not refuted, just not covered by a surviving verified claim — confirm exact behavior before building the relevant phase. Context: `.planning/research/SOLIDWORKS-UI.md`.

- [ ] **Heads-up View toolbar default roster** — exact ordered default buttons (SW2024/2025) and how the View Orientation flyout enumerates standard views / view cube / Spacebar dialog. *(blocks CANVAS-01, CANVAS-02)*
- [ ] **FeatureManager interactions** — official docs for rollback-bar drag (suppress-below vs insert-mid-history), F2 rename, double-click-to-edit, drag-to-reorder, FeatureManager flyout. *(blocks TREE-01..04)*
- [ ] **Mouse-gesture vs right-click threshold** — precise drag distance/time that distinguishes a gesture from a context-menu right-click, and behavior on release inside vs outside the guide (the "release-inside-cancels" mechanism was refuted 1-2). *(blocks CANVAS-06)*
- [ ] **Confirmation corner & successors** — top-right ✓/✗ behavior, the D-key "move dialog to cursor", and how newer graphics-area pushbuttons relate to / supersede the legacy confirmation corner. *(blocks CANVAS-03)*
- [ ] **Selection breadcrumbs (SW2018+)** — exact breadcrumb content/behavior and how it composes with the on-selection context toolbar. *(blocks CANVAS-04)*
- [ ] **Selection filters + F-key bindings** — the canonical filter toolbar contents and default F-key map. *(blocks NAV-04)*
- [ ] **Hover pre-selection highlight** — SW highlight color/feel spec to match. *(blocks NAV-03)*
- [ ] **Instant3D drag handles** — documented handle set and live-edit behavior. *(blocks CANVAS-07)*

## Architecture (from project research)

- [ ] **Ribbon: native vs SARibbon** — does native `QTabWidget` + `ToolBarAreaWidget` reach full SW polish (contextual tabs, galleries, app button), or is SARibbon (MIT, v2.8.0) warranted? Spike early. *(blocks RIBBON-01)* — see `.planning/research/STACK.md`, `SUMMARY.md`.
- [ ] **Body.Tip suppress-below code path** — confirm the PartDesign tip/recompute path can back the SW rollback bar UX. *(blocks TREE-02)*
- [ ] **SoFCUnifiedSelection box-select routing** — can FreeCAD `BoxSelection` be extended for enclose-vs-cross semantics, or does it need replacement? *(blocks NAV-02)*

---
*Created: 2026-06-06 during /gsd-new-project requirements validation*
