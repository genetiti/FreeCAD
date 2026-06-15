# Requirements: FreeCAD SolidWorks-Style UI

**Defined:** 2026-06-06
**Core Value:** A SolidWorks user can open FreeCAD and be immediately productive — it looks, navigates, and behaves like SolidWorks — with no FreeCAD tutorial required.

> Scope note: These requirements cover the **interaction/presentation** layer that constitutes "the SolidWorks experience." FreeCAD's modeling engine (Part/PartDesign/Sketcher + OpenCASCADE) already provides the geometry power and is recorded as Validated in PROJECT.md. Every requirement below is a UI/UX behavior a SolidWorks user reaches for by muscle memory. Feature IDs (T#/D#) reference `.planning/research/FEATURES.md`.

## v1 Requirements

The user chose **full parity** for v1 — the complete SolidWorks part-modeling experience (research tiers P1 + P2 + P3). The roadmap stages these across phases; all are in scope for the milestone.

> **Validated against official SolidWorks Help** via a deep-research pass (24/25 claims confirmed) — see `.planning/research/SOLIDWORKS-UI.md`. Behavior wording, official terminology ("box selection" / "cross selection"; pink active selection boxes; gesture directions 2/3/4/8/12 default 4), and three previously-missed elements (Task Pane, magnifying glass, reference triad) were folded in below. Items flagged for version-pinned re-verification at plan time are listed in `.planning/research/questions.md`.

### Shell & Activation

- [x] **SHELL-01**: User can launch FreeCAD into a "SolidWorks mode" that applies the full SolidWorks-style interface (ribbon, left tree, left PropertyManager, SW navigation) as one coherent layout
- [x] **SHELL-02**: The SolidWorks-style interface builds and runs on Windows, macOS, and Linux

### CommandManager (Ribbon)

- [x] **RIBBON-01**: User sees a tabbed CommandManager ribbon at the top (Features / Sketch / Evaluate / …) with large labeled icons and flyout split-buttons, driven by FreeCAD's command registry *(T1)*
- [x] **RIBBON-02**: The active ribbon tab switches by context (e.g. the Sketch tab activates on entering a sketch) *(T1/T10)*

### FeatureManager Design Tree

- [x] **TREE-01**: User sees a left-docked FeatureManager design tree showing features in creation order, with an origin/planes node and sketches nested under their features *(T2)*
- [x] **TREE-02**: User can drag a rollback bar in the tree to roll the model back to an earlier feature state and insert/edit mid-history *(T3)*
- [ ] **TREE-03**: User can double-click a feature in the tree to open it for editing in the PropertyManager, and F2 to rename *(T8)* — F2-rename half delivered in 03-02 (inherited relabel action live on the mounted FwFeatureTree); double-click-to-PropertyManager half remains for Phase 4
- [x] **TREE-04**: User can drag features up/down in the tree to reorder history, with validity feedback *(T9)*

### PropertyManager

- [ ] **PROP-01**: When the user starts a feature or sketch command, a PropertyManager panel slides in on the left with a green-✓/red-✗ header, collapsible rollout groups, live-updating fields, and dynamic options; the model previews live and the panel is non-modal (geometry still pickable) *(T4, A5)*
- [ ] **PROP-02**: PropertyManager uses SolidWorks-style selection-reference boxes — **pink when active** (blue is reserved for prompt icons, per official Help) — that the user clicks into and fills by picking geometry; selecting an item highlights it in the graphics area, the box auto-expands, and supports clear/remove *(D5)*

### Navigation & Selection

- [x] **NAV-01**: SolidWorks mouse navigation is the default — rotate = MMB drag, pan = Ctrl+MMB (no Ctrl needed in drawings), zoom = scroll wheel with zoom-to-cursor, roll = Alt+MMB, dolly = Shift+MMB, and middle-click an entity then middle-drag to rotate about it *(T5)*
- [ ] **NAV-02**: User selects with SolidWorks semantics — left-click face/edge/vertex, click-empty deselects, Ctrl-click multi-select, **box selection** (drag left→right = items fully enclosed) and **cross selection** (drag right→left = items crossing the boundary *plus* those enclosed) *(T6)*
- [ ] **NAV-03**: Hovering a face/edge/vertex pre-selection-highlights it (SW color/feel) and shows the element name *(T7)*
- [ ] **NAV-04**: User can toggle selection filters (faces/edges/vertices) via a floating filter toolbar and F-key bindings *(D6)*

### Modeling Flow

- [ ] **FLOW-01**: Sketch→feature flow matches SolidWorks — pick a plane/face → Sketch → draw → exit → the finished sketch is auto-selected as the profile and the Features tab is ready for Extrude/Revolve/etc. *(T10)*

### On-Canvas Interface

- [ ] **CANVAS-01**: A semi-transparent heads-up view toolbar floats at the top of the graphics area (view orientation, display style, hide/show, section, zoom-to-fit) *(T11)*
- [ ] **CANVAS-02**: User has quick standard-view buttons + a corner view-orientation cube + a corner reference triad, with spacebar opening the orientation menu *(T12)*
- [ ] **CANVAS-03**: A confirmation corner (translucent ✓/✗) appears in the top-right of the graphics area while in a mode, accepting/cancelling the active edit; the D key brings it to the cursor *(D3)*
- [ ] **CANVAS-04**: Selecting geometry pops a context mini-toolbar near the cursor with likely next commands, plus a breadcrumb trail (Body › Feature › Face) for selecting up the hierarchy — without breaking the right-click context menu *(D4, A4)*
- [ ] **CANVAS-05**: Pressing `S` opens a context-aware shortcut toolbar at the cursor (different command sets for part/sketch), user-customizable *(D1)*
- [ ] **CANVAS-06**: Holding RMB + dragging fires a mapped command via a radial gesture guide configurable to 2/3/4/8/12 directions (default 4), using a drag-distance threshold so a plain right-click still opens the context menu *(D2, A4)*
- [ ] **CANVAS-07**: User can drag on-canvas handles (Instant3D) to change extrude depth, fillet radius, etc. directly in the viewport *(D7)*
- [ ] **CANVAS-08**: User can invoke a magnifying glass (G key) to inspect/select detail without changing the zoom level *(SW Magnifying Glass)*

### Task Pane (Right Sidebar)

- [ ] **PANE-01**: User has a right-side Task Pane with collapsible tabs — Design Library, Appearances/Scenes/Decals, Custom Properties, and resources — matching SolidWorks' persistent right sidebar *(SW Task Pane)*

### Visual Theme

- [ ] **THEME-01**: The interface uses a recreated **look-alike** SolidWorks visual theme — icon style, blue accent, panel chrome, and fonts — built from original assets (no copied proprietary art) *(D8, A1)*

## v2 Requirements

Deferred to a future milestone. Tracked but not in the current roadmap.

### Additional CAD Contexts

- **ASM-01**: SolidWorks-style Assembly interface (mate PropertyManager, assembly ribbon tab)
- **DRW-01**: SolidWorks-style Drawing interface (sheets/views via TechDraw, drawing ribbon tab)
- **SHM-01**: SolidWorks-style Sheet Metal interface

### Interoperability

- **INTEROP-01**: SolidWorks file round-trip (.sldprt / .sldasm import/export) — a geometry/interop effort, separate from UI parity

## Out of Scope

Explicitly excluded. Documented to prevent scope creep. (See `.planning/research/FEATURES.md` anti-features A1–A7.)

| Feature | Reason |
|---------|--------|
| Verbatim SolidWorks proprietary assets (exact icons, theme files, branding) | Copyright/trademark risk. Layout/behavior mimicry is fine; copying the art is not. Use recreated look-alikes (THEME-01). |
| Replicating SolidWorks bugs/quirks "for faithfulness" | Users have muscle memory for intended behavior, not defects. Reproduce intent, fix quirks quietly. |
| Fully modal PropertyManager that locks the UI | SW's panel is semi-modal; geometry stays pickable. Hard-modal would break selection-box picking. (PROP-01 stays non-modal.) |
| Exhaustive legacy floating-toolbar customization sprawl | Low value once ribbon + S-key + gestures + heads-up bar cover fast access. |
| Assembly / Drawing / Sheet-Metal UI in v1 | Large separate contexts; shipping all at once blocks validating the core part-modeling experience. Deferred to v2. |
| SolidWorks file-format round-trip in the UI milestone | Geometry/interop problem, not a UI problem. Deferred to v2. |

## Traceability

Each v1 requirement maps to exactly one phase. See `.planning/ROADMAP.md` for phase detail.

| Requirement | Phase | Status |
|-------------|-------|--------|
| SHELL-01 | Phase 1 | Complete |
| SHELL-02 | Phase 1 | Complete |
| NAV-01 | Phase 1 | Complete |
| RIBBON-01 | Phase 2 | Complete |
| RIBBON-02 | Phase 2 | Complete |
| TREE-01 | Phase 3 | Complete |
| TREE-02 | Phase 3 | Complete |
| TREE-04 | Phase 3 | Complete |
| PROP-01 | Phase 4 | In Progress (04-01 Wave-0 logic + D-03 engine) |
| PROP-02 | Phase 4 | In Progress (04-01 Wave-0 logic + D-03 engine) |
| TREE-03 | Phase 4 | In Progress (04-01 Wave-0 logic + D-03 engine) |
| FLOW-01 | Phase 4 | In Progress (04-01 Wave-0 logic + D-03 engine) |
| NAV-02 | Phase 5 | Pending |
| NAV-03 | Phase 5 | Pending |
| NAV-04 | Phase 5 | Pending |
| CANVAS-01 | Phase 5 | Pending |
| CANVAS-02 | Phase 5 | Pending |
| CANVAS-03 | Phase 5 | Pending |
| CANVAS-04 | Phase 5 | Pending |
| CANVAS-05 | Phase 5 | Pending |
| CANVAS-08 | Phase 5 | Pending |
| CANVAS-06 | Phase 6 | Pending |
| CANVAS-07 | Phase 6 | Pending |
| THEME-01 | Phase 7 | Pending |
| PANE-01 | Phase 7 | Pending |

**Coverage:**

- v1 requirements: 25 total (enumerated count — the "23" cited during init was a miscount; the v1 checkbox set is SHELL×2, RIBBON×2, TREE×4, PROP×2, NAV×4, FLOW×1, CANVAS×8, PANE×1, THEME×1 = 25)
- Mapped to phases: 25 (100%)
- Unmapped: 0

---
*Requirements defined: 2026-06-06*
*Last updated: 2026-06-06 after roadmap creation (traceability populated)*
