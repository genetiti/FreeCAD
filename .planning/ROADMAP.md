# Roadmap: FreeCAD SolidWorks-Style UI

## Overview

This milestone turns FreeCAD's Qt6 Gui layer into a SolidWorks-faithful part-modeling interface — a fork that *looks*, *navigates*, and *behaves* like SolidWorks while leaving the App/OCCT modeling engine untouched. The journey starts by laying upstream-merge and legal-asset discipline (the non-negotiable foundation), then ships the free "SW feel" win (navigation default) inside a registrable SolidWorks-mode module shell. From there each phase restyles and repositions an existing FreeCAD Gui subsystem into its SolidWorks analog: the CommandManager ribbon (top chrome), the FeatureManager design tree with rollback bar (left), the keystone PropertyManager slide-in panel (left, the spine of all feature creation/editing), the selection model and on-canvas accelerators that make it *feel* like SolidWorks, the advanced gesture/Instant3D power-user layer, and finally a dedicated look-alike visual theme + Task Pane with a legal-review checkpoint before any public distribution. Every phase is an end-to-end, usable SolidWorks-feel increment — never a horizontal technical layer — and the whole thing rides FreeCAD's existing extension seams (`SwWorkbench`, `DockWindowManager`, `Control`/`TaskView`, `OverlayManager`) to stay mergeable against a moving upstream `main`.

## Phases

**Phase Numbering:**

- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [x] **Phase 1: SolidWorks Mode Foundation** - Additive module shell, upstream-merge + asset-provenance discipline, CI gates, and SW navigation as the default — launch into a coherent SolidWorks-mode skeleton that *navigates* like SW (completed 2026-06-07)
- [x] **Phase 2: CommandManager Ribbon** - Tabbed top ribbon (Features/Sketch/Evaluate) with large labeled icons, flyouts, and context-driven tab switching, driven by FreeCAD's command registry (completed 2026-06-14)
- [ ] **Phase 3: FeatureManager Design Tree** - Left-docked ordered feature tree with origin/planes node, draggable rollback bar, F2 rename, and drag-to-reorder
- [ ] **Phase 4: PropertyManager Panel** - Left slide-in command panel with green-✓/red-✗ header, collapsible rollouts, pink selection-reference boxes, live preview, in-tree double-click edit, and the sketch→feature flow
- [ ] **Phase 5: Selection Parity & On-Canvas Accelerators** - SW box/cross selection, hover pre-highlight, filter toolbar, heads-up view toolbar, view cube/triad, confirmation corner, context mini-toolbar/breadcrumb, S-key bar, and magnifying glass
- [ ] **Phase 6: Mouse Gestures & Instant3D Handles** - RMB radial gesture guide (threshold-protected) and on-canvas drag handles for live dimension editing
- [ ] **Phase 7: Visual Theme, Task Pane & Legal Sign-off** - Complete look-alike SVG icon/QSS theme, right-side Task Pane, cross-platform/HiDPI QA, and IP legal review checkpoint

## Phase Details

### Phase 1: SolidWorks Mode Foundation

**Goal**: A SolidWorks user can launch FreeCAD into a registrable "SolidWorks mode" that applies a coherent SW-style shell and navigates with SolidWorks mouse conventions on Windows, macOS, and Linux — built on an additive module with upstream-merge and asset-provenance discipline so all later work stays mergeable.
**Mode:** mvp
**Depends on**: Nothing (first phase)
**Requirements**: SHELL-01, SHELL-02, NAV-01
**Success Criteria** (what must be TRUE):

  1. User launches FreeCAD and activates "SolidWorks mode" (a `SwWorkbench`) that mounts the SW dock layout shell — the entry seam for the ribbon, left tree, and left PropertyManager added in later phases — with zero edits to `MainWindow.cpp` bodies beyond marked `// SW-FORK HOOK` lines
  2. The SolidWorks-style shell builds and runs on Windows, macOS, and Linux (cross-platform CI green; manual launch verified on all three)
  3. SolidWorks navigation is the fork default: rotate = MMB drag, pan = Ctrl+MMB, zoom = scroll-wheel zoom-to-cursor, roll = Alt+MMB, dolly = Shift+MMB, and middle-clicking an entity then middle-dragging rotates about it — with an explicit macOS no-middle-button / trackpad profile
  4. A CI asset-provenance guard rejects any binary image lacking a source entry in `ASSET_PROVENANCE.md`, and a headless/`--console` `.FCStd`-compat gate proves no GUI state leaks into the App layer
  5. A scripted upstream-sync drill against a pinned upstream commit runs and completes in hours not days, with all unavoidable shared-file touch points greppable via `// SW-FORK HOOK`**Plans**: 4 plans
- [x] 01-01-PLAN.md — Walking Skeleton: FreeWorks module + FwWorkbench + Fw_* dock shell (SHELL-01)
- [x] 01-02-PLAN.md — SolidWorks navigation default + macOS substitute profile (NAV-01)
- [x] 01-03-PLAN.md — Cross-platform build + headless .FCStd-compat gate (SHELL-02)
- [x] 01-04-PLAN.md — Merge-safety, asset provenance & trademark discipline (SHELL-02)

**UI hint**: yes

### Phase 2: CommandManager Ribbon

**Goal**: A SolidWorks user sees a familiar tabbed CommandManager ribbon at the top of the window with large labeled icons and flyout split-buttons, and the active tab switches by context exactly as SolidWorks does — so the app reads as "this is SolidWorks" at first glance.
**Mode:** mvp
**Depends on**: Phase 1
**Requirements**: RIBBON-01, RIBBON-02
**Success Criteria** (what must be TRUE):

  1. User sees a tabbed ribbon (Features / Sketch / Evaluate / …) mounted in the top toolbar area with large text-labeled icons, replacing FreeCAD's menus+toolbars in SolidWorks mode
  2. Commands on the ribbon are driven by FreeCAD's existing command registry (`ToolBarItem`/`MenuItem` trees consumed as data) and fire via `CommandManager` — no duplicated command backend
  3. Flyout split-buttons work (e.g. a Fillet button reveals Chamfer and related commands)
  4. The active ribbon tab switches by context — entering a sketch activates the Sketch tab — wired to `Control` active-dialog / edit state
  5. Ribbon tab layout persists across restarts (panel positions and tab state restore correctly), built as a self-contained widget that survives an upstream sync without merge conflict

**Plans**: 4 plans
**Wave 1**

- [x] 02-01-PLAN.md — Wave 0 test scaffold + native ribbon spike gate (native-vs-SARibbon verdict → **native committed**) (RIBBON-01)

**Wave 2** *(blocked on Wave 1 completion)*

- [x] 02-02-PLAN.md — Curated declarative map + full FwRibbon build (tabs, large labeled buttons, flyouts, auto-derive) (RIBBON-01)

**Wave 3** *(blocked on Wave 2 completion)*

- [x] 02-03-PLAN.md — Mount in top area (real QToolBar Fw_RibbonToolBar), reversible chrome hiding, discoverability escape hatch, two-layer persistence (RIBBON-01)

**Wave 4** *(blocked on Wave 3 completion)*

- [x] 02-04-PLAN.md — Context tab switching: sketch-edit activates the Sketch tab, restores on exit (RIBBON-02)

**UI hint**: yes

### Phase 3: FeatureManager Design Tree

**Goal**: A SolidWorks user sees and works with a left-docked FeatureManager design tree — features in creation order under an origin/planes node, sketches nested under their features — and can roll the model back to an earlier state with a draggable rollback bar, rename with F2, and drag features to reorder history with validity feedback.
**Mode:** mvp
**Depends on**: Phase 1
**Requirements**: TREE-01, TREE-02, TREE-04
**Success Criteria** (what must be TRUE):

  1. User sees a left-docked, SolidWorks-styled feature tree showing features in creation order with an origin/planes node at top and each sketch nested under its parent feature
  2. User drags a rollback bar up the tree to roll the model back to an earlier feature state (suppress-below, recompute to that point) and can insert/edit mid-history — backed by the existing `Body.Tip` engine, with rollback state held entirely in the Gui layer
  3. User presses F2 on a tree feature to rename it, and the new name propagates correctly
  4. User drags a feature up/down in the tree to reorder history with a visible insertion-line affordance and validity feedback, and the reorder goes through document transactions (clean undo/redo, no `.FCStd` pollution)
  5. A daily-SolidWorks-user acceptance check confirms rollback suppress-below + insert-at-bar *feels* like SolidWorks, not just looks like it

**Plans**: 3 plans
**Wave 1**

- [ ] 03-01-PLAN.md — Wave 0 test scaffold + tree-reuse spike gate (D-03 `reuse committed`/`subclass-fallback committed` verdict; Body.Tip rollback-logic confirmed) (TREE-01, TREE-02, TREE-04)

**Wave 2** *(blocked on Wave 1 completion)*

- [ ] 03-02-PLAN.md — Mount FwFeatureTree in Fw_FeatureManager: active-Body-scoped tree, Origin/Front-Top-Right planes, nested sketches, F2 rename, drag-to-reorder with validity BLOCK (TREE-01, TREE-04, TREE-03 F2)

**Wave 3** *(blocked on Wave 2 completion)*

- [ ] 03-03-PLAN.md — Rollback bar: drawn band drives the real Body.Tip (suppress-below + recompute, no module link), reversible Roll Back/Forward/To End, insert-at-bar, Gui-only greying + live checklist (TREE-02)

**UI hint**: yes

### Phase 4: PropertyManager Panel

**Goal**: A SolidWorks user runs a feature or sketch command and a left-side PropertyManager slides in — green-✓/red-✗ header, collapsible rollout groups, pink active selection-reference boxes, live-updating fields, live model preview, and full SolidWorks keyboard/focus semantics — and can double-click a tree feature to edit it there, completing the sketch→feature modeling loop.
**Mode:** mvp
**Depends on**: Phase 3
**Requirements**: PROP-01, PROP-02, TREE-03, FLOW-01
**Success Criteria** (what must be TRUE):

  1. When the user starts a feature/sketch command, a PropertyManager panel slides in on the **left** with a green-✓ accept / red-✗ cancel header, collapsible rollout groups, and dynamic options that appear/disappear by choice — non-modal so geometry stays pickable, with the model previewing live
  2. PropertyManager selection-reference boxes are **pink when active** (blue reserved for prompt icons); the user clicks into a box, picks geometry which fills and highlights it, the box auto-expands, and clear/remove work
  3. User double-clicks a feature in the tree to open it for editing in the PropertyManager (routes through the existing `setEdit()` path into the left panel)
  4. Enter/Escape/Tab/focus behave per SolidWorks (not FreeCAD defaults), and every edit is transaction-wrapped so undo/redo is clean; existing PartDesign task panels work unmodified inside the new container
  5. Sketch→feature flow matches SolidWorks: pick a plane/face → Sketch → draw → exit → the finished sketch is auto-selected as the profile and the Features tab is ready for Extrude/Revolve

**Plans**: TBD
**UI hint**: yes

### Phase 5: Selection Parity & On-Canvas Accelerators

**Goal**: A SolidWorks user selects, filters, and drives the viewport exactly as in SolidWorks — directional box/cross selection, hover pre-highlight, a filter toolbar, a heads-up view toolbar, view cube/triad, a confirmation corner, an on-selection context mini-toolbar with breadcrumb, the S-key shortcut bar, and the G-key magnifying glass — closing the interaction loop that connects tree, panel, and canvas.
**Mode:** mvp
**Depends on**: Phase 4
**Requirements**: NAV-02, NAV-03, NAV-04, CANVAS-01, CANVAS-02, CANVAS-03, CANVAS-04, CANVAS-05, CANVAS-08
**Success Criteria** (what must be TRUE):

  1. User box-selects left→right and gets only fully-enclosed items (box selection); drags right→left and gets crossing items plus enclosed ones (cross selection); left-click picks face/edge/vertex, click-empty deselects, Ctrl-click multi-selects
  2. Hovering a face/edge/vertex pre-selection-highlights it in SolidWorks color/feel and shows the element name; selection filters (faces/edges/vertices) toggle via a floating filter toolbar and F-key bindings
  3. A semi-transparent heads-up view toolbar floats at the top of the graphics area (view orientation, display style, hide/show, section, zoom-to-fit), plus corner view-cube + reference triad and spacebar opening the orientation menu
  4. A translucent confirmation corner (✓/✗) appears top-right while in a mode and accepts/cancels the active edit, with the D key bringing it to the cursor; selecting geometry pops a context mini-toolbar near the cursor with likely next commands and a breadcrumb (Body › Feature › Face) — without breaking the right-click context menu
  5. Pressing `S` opens a context-aware shortcut toolbar at the cursor (different sets for part vs sketch); pressing `G` invokes a magnifying glass to inspect/select detail without changing zoom — all on-canvas overlays are leak-checked (ASan/Valgrind clean over a long session)

**Plans**: TBD
**UI hint**: yes

### Phase 6: Mouse Gestures & Instant3D Handles

**Goal**: A SolidWorks power user invokes commands by holding the right mouse button and flicking through a radial gesture guide, and edits dimensions by dragging on-canvas handles directly in the viewport — the highest-muscle-memory accelerators, isolated here because they carry the most UX risk (context-menu conflict, general handle system).
**Mode:** mvp
**Depends on**: Phase 5
**Requirements**: CANVAS-06, CANVAS-07
**Success Criteria** (what must be TRUE):

  1. Holding RMB + dragging fires a mapped command via a radial gesture guide configurable to 2/3/4/8/12 directions (default 4), per-context profiles for part vs sketch
  2. A plain right-click (or sub-threshold drag) still opens the context menu — the gesture detector uses a drag-distance/time threshold so it never swallows the context menu
  3. User drags on-canvas Instant3D handles to change extrude depth, fillet radius, and similar dimensions directly in the viewport, with edits transaction-wrapped and live-previewing
  4. Gesture and Instant3D Coin3D scene-graph additions use RAII node ownership (built in `attach()`, torn down in `detach()`) and pass a leak check over a long session

**Plans**: TBD
**UI hint**: yes

### Phase 7: Visual Theme, Task Pane & Legal Sign-off

**Goal**: A SolidWorks user sees a complete look-alike SolidWorks visual theme (icon style, blue accent, panel chrome, fonts) and a familiar right-side Task Pane — all built from original assets — and the milestone clears an IP legal-review checkpoint and per-platform/per-scale visual QA before any public distribution.
**Mode:** mvp
**Depends on**: Phase 5
**Requirements**: THEME-01, PANE-01
**Success Criteria** (what must be TRUE):

  1. The interface uses a recreated look-alike SolidWorks theme — icon style, blue accent, panel chrome, fonts — applied via a QSS preference pack and SVG icons through `BitmapFactory`, with every shipped icon traced to an original source in `ASSET_PROVENANCE.md` and zero proprietary assets in the repo or git history
  2. User has a right-side Task Pane with collapsible tabs — Design Library, Appearances/Scenes/Decals, Custom Properties, and resources — matching SolidWorks' persistent right sidebar
  3. The theme passes per-platform + per-scale visual QA (100/150/200% and Retina): ribbon and tree icons are crisp, panel metrics correct, no macOS QSS/native-style breakage
  4. No "SolidWorks" wordmark, logo, or string appears in any UI string, window title, About box, or installer; the product carries a distinct name
  5. An IP-counsel review of the visual theme and marketing copy is completed and signed off as release-ready

**Plans**: TBD
**UI hint**: yes

## Progress

**Execution Order:**
Phases execute in numeric order: 1 → 2 → 3 → 4 → 5 → 6 → 7

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. SolidWorks Mode Foundation | 4/4 | Complete   | 2026-06-07 |
| 2. CommandManager Ribbon | 4/4 | Complete    | 2026-06-14 |
| 3. FeatureManager Design Tree | 0/3 | Planned | - |
| 4. PropertyManager Panel | 0/TBD | Not started | - |
| 5. Selection Parity & On-Canvas Accelerators | 0/TBD | Not started | - |
| 6. Mouse Gestures & Instant3D Handles | 0/TBD | Not started | - |
| 7. Visual Theme, Task Pane & Legal Sign-off | 0/TBD | Not started | - |
