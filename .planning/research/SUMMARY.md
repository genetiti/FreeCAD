# Project Research Summary

**Project:** FreeCAD: SolidWorks-Style UI
**Domain:** CAD desktop application — SolidWorks-faithful UI/UX fork of FreeCAD's Qt6/C++ Gui layer
**Researched:** 2026-06-06
**Confidence:** HIGH

## Executive Summary

This project is a brownfield GUI fork: the FreeCAD App layer (OCCT geometry, parametric DOM, PartDesign/Sketcher) stays completely frozen, and every deliverable lives in `src/Gui/` and `src/Mod/*/Gui/`. The most important finding across all four research streams is that **the majority of SolidWorks' experience already exists in FreeCAD as working building blocks** — `SolidWorksNavigationStyle` ships in-tree, `OverlayManager`/`TaskView`/`Control` is a near-1:1 analog of the PropertyManager, `TreeWidget` is an ordered document-synced feature tree, and the `Body.Tip` engine is the rollback suppression primitive. The milestone is dominated by **presentation and interaction-shell work**, not geometry or engine work. The roadmapper should treat this as assembly and restyling of existing FreeCAD machinery, with one net-new high-complexity build (the CommandManager ribbon) and several medium-complexity interaction-semantic upgrades (selection directionality, PropertyManager modality, rollback-bar drag UI).

The recommended architecture is an additive `src/Gui/SolidWorks/` module activated via a `Gui::Workbench` subclass (`SwWorkbench`). This rides FreeCAD's existing workbench activation machinery (stable, tested) and keeps all SW-specific code out of `MainWindow.cpp`, `Tree.cpp`, and `View3DInventorViewer.cpp` — the three most actively churned shared files. The ribbon is the main open decision: build natively on `ToolBarAreaWidget`/`QTabWidget` (zero new deps, cleanest upstream tracking) or adopt SARibbon MIT v2.8.0 (vendored submodule) if native ribbon cannot reach SW visual polish within schedule. No other new dependencies are warranted; the existing FreeCAD dock/overlay/theme stack covers everything else.

The dominant risks are not technical — they are **process and legal**. Copying verbatim SolidWorks icon/theme assets (even "temporarily") creates permanent IP liability and must be enforced with CI asset-provenance guards from day one. Deep in-place edits to shared `src/Gui/` files create permanent merge hell against a moving upstream target; the mitigation is the additive-module architecture and frequent (biweekly) upstream syncs with `// SW-FORK HOOK` markers at every unavoidable touch point. The third risk is superficial UX parity: the UI can look like SolidWorks but feel wrong if selection directionality, PropertyManager keyboard semantics, and rollback suppress-below behavior are not spec'd and acceptance-tested with real SolidWorks users.

---

## Key Findings

### Recommended Stack

The entire project builds on technologies already present in FreeCAD's `main`. No new dependencies are required on the recommended (native) path. The one optional addition is SARibbon (MIT, v2.8.0, vendored submodule) as a fallback if a native ribbon cannot reach SW polish. Everything else — docking, slide-in panels, theming, icons, navigation, selection — is handled by existing FreeCAD Gui infrastructure.

**Core technologies (all already present):**
- **Qt 6.8 Widgets**: All chrome stays in QtWidgets end-to-end; zero new deps, fully cross-platform, upstream-merge-friendly.
- **`Gui::SolidWorksNavigationStyle`**: SW rotate/pan/zoom already ships in-tree; just make it the fork default.
- **`Gui::OverlayManager` / `OverlayWidgets`**: Existing slide-in / auto-hide side-panel system — the PropertyManager container behavior for free.
- **`Gui::Control` + `TaskView` / `TaskDialog`**: Existing command-scoped option panel — the PropertyManager content mechanism, already wired to transactions and live preview.
- **`Gui::TreeWidget`** (`QTreeWidget` + `SelectionObserver` + `DocumentObserver`): Ordered, document-synced, drag-capable feature tree — FeatureManager subclassing target.
- **`Gui::WorkbenchManipulator` / `Gui::Workbench`**: Additive extension seam for installing the SW layout via existing activation machinery.
- **`Gui::ToolBarAreaWidget` + `ToolBarManager`**: 2024-era custom toolbar-area widget — the native foundation for a ribbon shell.
- **`StyleParameters` + QSS + `PreferencePackManager`**: Parametric theming engine; ship a "SolidWorks" preference pack.
- **`Gui::BitmapFactory` + SVG `.qrc`**: Icon pipeline; recreate look-alike SVGs — never copy SW assets.
- **SARibbon v2.8.0 (MIT)** *(conditional)*: Only if native ribbon stalls on SW polish; vendor as `src/3rdParty/SARibbon` submodule.

**Do not add:** Qt-Advanced-Docking-System (duplicates existing dock/overlay stack), QtitanRibbon (commercial, LGPL-incompatible), QxRibbon (unmaintained), QML (wrong widget system), hard-coded `setStyleSheet()` calls.

### Expected Features

FreeCAD already implements most SW UI primitives. The gap is interaction shells: ribbon frontend, SW-positioned/SW-styled panels, and on-canvas accelerators. See `.planning/research/FEATURES.md` for the full table.

**Must have — table stakes (v1):**
- **T5 SW mouse navigation as fork default** — already built; flip preference + verify zoom-to-cursor feel. Zero-cost first win.
- **T1 CommandManager ribbon** — tabbed top ribbon (Features/Sketch/Evaluate); the single most visible "this is SolidWorks" signal. Net-new widget, highest complexity.
- **T2 FeatureManager tree (left dock, SW-styled, ordered)** — mostly re-dock + re-skin of existing `TreeWidget`; left placement, SW icon/label conventions.
- **T4 PropertyManager (left slide-in, checkmark pin, collapsible groups)** — FreeCAD `TaskView`/`Control` is 70% there; re-dock left via `OverlayManager`, SW-style chrome.
- **T3 Rollback bar** — draggable bar in the tree; `Body.Tip` engine (suppress-below) exists; only the drag-bar UI is missing. High user value.
- **T6 Selection model** — SW box-select directionality (enclosed vs crossing), filter toolbar, F-key bindings over existing `SelectionFilter` engine.
- **T7 Hover preselection highlight** — mostly present; SW highlight color/feel.
- **T8 In-tree feature editing** — double-click to PropertyManager; mostly present given T4.
- **T9 Drag-to-reorder features** — mostly present; SW insertion-line visual affordance.
- **T10 Sketch to feature flow** — wire ribbon tab-switch + auto-select after sketch exit.
- **T11 Heads-up view toolbar** — floating overlay at top of 3D view; uses `OverlayManager` infra.
- **T12 View cube / standard views** — NaviCube already exists; spacebar binding + SW styling gap.

**Should have — power-user differentiators (v1.x after validation):**
- **D5 Selection-reference boxes** — reusable `SelectionBoxWidget` (blue/pink pick lists) inside PropertyManager panels.
- **D1 S-key shortcut bar** — context-aware popup at cursor; top SW power-user accelerator; self-contained.
- **D3 Confirmation corner** — on-canvas checkmark/X overlay tied to active TaskDialog; D-key teleport.
- **D6 Filter toolbar** — floating F5/F6/F-key selection-filter bar wired to existing `SelectionFilter`.
- **D8 Visual theme (look-alike icons/colors)** — ongoing across all phases.

**Defer to v2+:**
- **D2 Mouse gestures (RMB radial donut)** — context-menu conflict risk; defer until selection is rock-solid.
- **D4 Context toolbar + breadcrumb** — on-selection mini toolbar + selection-chain breadcrumb.
- **D7 Instant3D on-canvas handles** — large effort; only after the whole shell feels like SW.
- **Assembly/Drawing/Sheet-Metal SW UI contexts** — extend shells to new contexts after part-modeling is validated.

**Deliberately not built:**
- Verbatim SW proprietary assets (legal constraint)
- SW file-format round-trip (out of scope per PROJECT.md)
- Hard-modal PropertyManager that blocks viewport interaction (do not regress from FreeCAD's non-modal Task panel)

### Architecture Approach

The entire SW presentation layer lives in a new additive `src/Gui/SolidWorks/` module. A `SwWorkbench` (`Gui::Workbench` subclass) is the entry point: its `activated()` override mounts the ribbon, registers new docks, and applies the SW layout via public `MainWindow`/`DockWindowManager` API. Every SW widget is a view — it observes existing signals and issues changes only via `Gui::Command`, property setters, and `Control().accept()`. The App DOM is never touched directly. See `.planning/research/ARCHITECTURE.md` for the full component map and data-flow diagrams.

**Major components (all under `src/Gui/SolidWorks/`):**
1. **`SwWorkbench` + `SwLayout`** — workbench subclass as mode entry point; dock layout installer; upstream merge anchor.
2. **`SwRibbon` / `SwRibbonTab` / `SwCommandBar`** — tabbed ribbon widget consuming existing `ToolBarItem`/`MenuItem` command trees; mounted in `MainWindow`'s `TopToolBarArea`.
3. **`SwFeatureManager` + `SwRollbackBar`** — `TreeWidget` subclass + rollback bar overlay; registered as a new dock name via `DockWindowManager`.
4. **`SwPropertyManager`** — left-dock host wrapping existing `Control`/`TaskView` + `PropertyEditor`; all existing per-feature task panels work unmodified inside it.
5. **`SwSelectionContextBar`** — observer widget on `SelectionSingleton` signals; drives context toolbar, filter bar, breadcrumb.
6. **`SwTheme`** — loads QSS preference pack + look-alike SVG icons via `BitmapFactory`.

### Critical Pitfalls

1. **Copying SolidWorks proprietary assets verbatim** — install a CI asset-provenance guard (`ASSET_PROVENANCE.md` + hash check) before any icon lands in the repo. Recovery requires full git-history purge — there is no easy fix after the fact.

2. **Deep edits to `MainWindow.cpp`, `Tree.cpp`, `View3DInventorViewer.cpp`** — these are upstream's most actively churned files; every in-place edit becomes a multi-day merge conflict on the next upstream sync. Use the additive `SwWorkbench`/`DockWindowManager` seam; tag every unavoidable shared-file touch with `// SW-FORK HOOK`.

3. **Superficial UX parity ("right at a glance, wrong in the feel")** — the ribbon and panels can look correct but feel wrong if SW selection directionality, PropertyManager Enter/Escape/focus semantics, and rollback suppress-below behavior are not spec'd as behavioral acceptance tests and validated by real daily SolidWorks users.

4. **Breaking the App/Gui separation** — SW UI state belongs in the Gui layer (ViewProvider properties, Gui controllers), never in `App::DocumentObject`. App-layer bleed pollutes `.FCStd`, breaks headless (`--console`) mode, and creates a second upstream-conflict surface.

5. **Bypassing Property change notification / mutating document during recompute** — always use `Property::setValue()` and `Document::openTransaction()/closeTransaction()`; never direct `obj->Member = x`; never `addObject`/`removeObject` inside `execute()`. Violations corrupt undo/redo and dependency tracking silently.

---

## Implications for Roadmap

The build order is determined by three hard constraints: (a) the additive module skeleton must exist before any panel can be hosted; (b) navigation is free and ships first to demonstrate immediate SW feel; (c) the ribbon is the highest-risk net-new build and its architecture decision (native vs SARibbon) must be resolved early before downstream phases depend on workbench-tab switching.

### Phase 0: Fork Scaffolding + Upstream-Merge Discipline
**Rationale:** Non-negotiable foundation. Every later phase inherits this discipline. Without it, any SW code touching shared files immediately accumulates permanent merge debt.
**Delivers:** `src/Gui/SolidWorks/` CMakeLists skeleton; `SwWorkbench` stub registered; upstream mirror branch; `ASSET_PROVENANCE.md`; CI gates (headless load, asset hash check); documented biweekly sync cadence with `// SW-FORK HOOK` convention; scripted upstream-sync drill.
**Addresses:** T5 navigation default (just a preference flip — trivially done here).
**Avoids:** Pitfall 3 (merge hell), Pitfall 1 (proprietary assets), Pitfall 4 (App/Gui boundary).

### Phase 1: SW Navigation Default + Module Shell + Theme Foundation
**Rationale:** Navigation default costs nothing and immediately demonstrates SW feel. The module shell must exist before any panel can be hosted. Theme is started here — establish QSS preference-pack architecture and SVG icon pipeline before panels multiply.
**Delivers:** `SolidWorksNavigationStyle` set as fork default; zoom-to-cursor + macOS/Linux cross-platform verification; `SwWorkbench` activating SW mode; QSS preference pack skeleton; SVG icon pipeline with provenance tracking.
**Uses:** `SolidWorksNavigationStyle` (existing), `StyleParameters`/`PreferencePackManager`, `BitmapFactory`, `WorkbenchManipulator`.
**Avoids:** Pitfall 8 (cross-platform navigation — test all three OSes in this phase), Pitfall 9 (HiDPI/QSS — establish SVG-first icon policy here).

### Phase 2: CommandManager Ribbon
**Rationale:** The ribbon is the single most visible "this is SolidWorks" signal and the only major net-new build. Build it before tree and panels because: (a) it exercises the command-tree consumption model that FeatureManager and context bars also rely on; (b) native-vs-SARibbon decision must be resolved before it blocks downstream phases; (c) workbench tab auto-switching (needed for T10) is a ribbon concern.
**Delivers:** Tabbed ribbon (Features/Sketch/Evaluate) replacing FreeCAD menus+toolbars; large icons with flyouts; command-grouping data structure; per-workbench tab mapping; layout save/restore; contextual tab switching wired to `Control` active-dialog state.
**Uses:** `ToolBarAreaWidget` (TopToolBarArea mount), `ToolBarManager`/`MenuItem` command trees, `Gui::CommandManager`. SARibbon (MIT, vendored) if native build stalls on polish.
**Avoids:** Pitfall 10 (ribbon vs Qt/upstream — self-contained widget, not in-place `MainWindow` edit), Pitfall 3 (keep `MainWindow.cpp` edits to one marked hook line).

### Phase 3: FeatureManager Design Tree
**Rationale:** After the ribbon establishes the top chrome, the FeatureManager anchors the left panel. The rollback bar is included here — not deferred — because the `Body.Tip` engine is already available at the App level and only the drag-bar UI is missing. Including it avoids a second tree-widget overhaul later.
**Delivers:** `SwFeatureManager` (TreeWidget subclass) left-docked, SW-styled, ordered flat feature list; `SwRollbackBar` drag bar over `Body.Tip` suppress-below engine; SW icon/label conventions; F2 rename; drag-to-reorder with insertion-line visual.
**Uses:** `Gui::TreeWidget` (subclass, not fork), `DockWindowManager::registerDockWindow()` (new dock name), `Body.Tip` / `ViewProviderBody.setTipIcon()`.
**Avoids:** Pitfall 7 (superficial rollback — spec suppress-below + insert-at-bar, validate with SW users), Pitfall 4 (rollback state stays in Gui), Pitfall 5 (feature reorder via transactions).

### Phase 4: PropertyManager (Left Slide-in Panel)
**Rationale:** T4 is the keystone of feature creation/editing — T8 (in-tree double-click edit) and T10 (sketch-to-feature flow) both terminate here. FreeCAD's `TaskView`/`Control` is the closest 1:1 analog; this phase re-docks it left, applies SW chrome, and adds the reusable `SelectionBoxWidget`. Existing per-feature task panels (PartDesign Pad, Pocket, etc.) work unmodified inside the new container.
**Delivers:** `SwPropertyManager` left-docked via `OverlayManager` slide-in; SW panel chrome (pin header, collapsible groups); `SelectionBoxWidget` reusable widget; correct Enter/Escape/Tab/focus SW semantics; green-checkmark accept / red-X cancel; undo/redo wrapping verified; existing PartDesign task panels working unmodified.
**Uses:** `Gui::Control` + `TaskView`/`TaskDialog` (hosted, not replaced), `OverlayManager` left-side auto-hide, `Gui::PropertyEditor`.
**Avoids:** Pitfall 7 (PropertyManager modality — spec and acceptance-test keyboard/focus semantics with SW users), Pitfall 5 (all writes via `Property::setValue()` + transactions).

### Phase 5: Selection Model Parity + Canvas Accelerators
**Rationale:** Selection is the connective tissue between all panels. After tree and property panel are established, this phase finalizes the interaction loop. Canvas accelerators (heads-up toolbar, confirmation corner, S-key bar) are co-located here because they all share the `OverlayManager` canvas-overlay pattern and are cheapest to build together after that pattern is proven by the PropertyManager.
**Delivers:** SW left-vs-right box-select semantics; `SwSelectionContextBar` observer; floating filter toolbar (F5/F6 bindings); heads-up view toolbar (top-of-canvas overlay); confirmation corner (checkmark/X, D-key teleport); S-key context-aware shortcut bar at cursor; hover preselection highlight SW colors; sketch-to-feature flow tab-switch + auto-select wired.
**Uses:** `SelectionSingleton::signalSelectionChanged`, `SelectionFilter`, `BoxSelection`, `SoFCUnifiedSelection`, `OverlayManager` canvas-overlay infra.
**Avoids:** Pitfall 7 (selection model — spec enclose-vs-crossing + pre-highlight; SW-user acceptance test), Pitfall 8 (cross-platform: macOS no-middle-button + Linux).

### Phase 6: Visual Theme + Legal Review Checkpoint
**Rationale:** Theme work is seeded in Phase 1 and refined throughout, but the full icon set and color pass belong to a dedicated phase after interaction shells are stable. The IP counsel review is a prerequisite for any public distribution.
**Delivers:** Complete recreated look-alike SVG icon set with `ASSET_PROVENANCE.md` coverage; final QSS preference pack (color scheme, fonts, panel metrics); per-platform + per-scale (100/150/200%, Retina) visual QA pass; legal IP review of theme + marketing copy completed; distinct product name (no "SolidWorks" in UI strings or window titles); release-readiness sign-off.
**Uses:** `BitmapFactory` + `.qrc`, `StyleParameters` + `FreeCAD.qss`, `PreferencePackManager`.
**Avoids:** Pitfall 1 (proprietary assets — final audit), Pitfall 2 (trade-dress overreach — IP counsel review), Pitfall 9 (QSS/HiDPI — SVG icons, surgical QSS, per-platform QA).

---

### Phase Ordering Rationale

- **Phase 0 before everything:** Upstream-merge discipline and CI gates must exist before any code lands.
- **Navigation first (Phase 1):** Free win, instant SW feel, proves the module skeleton works.
- **Ribbon before tree/property (Phase 2):** Highest net-new complexity; open architecture decision (native vs SARibbon) must be resolved before downstream phases depend on tab-switching.
- **Tree before PropertyManager (Phases 3 to 4):** PropertyManager "show props of selected feature" depends on tree selection being wired first.
- **Selection/canvas last of the core (Phase 5):** Cheapest to tune once both tree and property panel can react to selection signals.
- **Theme dedicated phase (Phase 6):** Icon work is wasted if done before panels stabilize; IP counsel review gates public distribution.

### Research Flags

Phases needing deeper research during planning:

- **Phase 2 (Ribbon):** Native-vs-SARibbon decision needs a short spike (1-2 days) — build a minimal native ribbon tab with contextual tab and flyout, assess SW visual fidelity gap before committing to either path. Also research SARibbon CMake submodule integration and macOS/Linux polish gaps.
- **Phase 3 (FeatureManager / Rollback):** The rollback drag interaction is the highest-risk UX. Research how `Body.Tip` suppression interacts with `execute()` ordering and mid-tree insert before writing behavioral specs.
- **Phase 5 (Selection parity):** `SoFCUnifiedSelection.cpp` (2,000+ lines) and the event-routing path for `BoxSelection` + `SelectionFilter` + `NavigationStyle` is complex. Research the exact box-select event routing before speccing SW directional semantics.

Phases with standard patterns (skip research-phase):

- **Phase 0 (Scaffolding):** Well-documented git mirror-branch and patch-series patterns; no domain-specific unknowns.
- **Phase 1 (Navigation default + shell):** Navigation style is already implemented; preference flip is documented; QSS preference pack pattern is established in FreeCAD.
- **Phase 4 (PropertyManager):** `Control`/`TaskView` reuse is well-understood; dock relocation via `OverlayManager` is documented infra.
- **Phase 6 (Theme + legal):** SVG icon authoring and QSS pack are standard; IP counsel review is a one-time external engagement.

---

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | Grounded in direct reads of working checkout (`768e237091`); SARibbon version/license verified against upstream repo June 2026. Native-vs-SARibbon question is intentional — resolved by spike in Phase 2. |
| Features | HIGH | SW feature set verified against official SW help and multiple training sources; FreeCAD current state verified directly against `src/Gui` and `src/Mod/PartDesign` source. |
| Architecture | HIGH | All extension seams (Workbench, DockWindowManager, ToolBarAreaWidget, Control, TreeWidget) verified against actual class/method signatures in the checkout. |
| Pitfalls | HIGH (architecture/upstream), MEDIUM (legal) | Architecture and upstream-merge pitfalls grounded in the codebase maps; legal pitfalls reflect public case law, not legal advice — IP counsel review recommended before distribution. |

**Overall confidence:** HIGH

### Gaps to Address

- **Native ribbon feasibility spike:** The largest technical uncertainty is whether a native `ToolBarAreaWidget`/`QTabWidget` ribbon can reach SW visual polish (contextual tabs, flyout split-buttons, panel collapse, application button). Plan a 1-2 day spike at the start of Phase 2 before committing to either path.
- **macOS middle-button / trackpad navigation profile:** `SolidWorksNavigationStyle` uses MMB for rotate and Ctrl+MMB for pan. macOS trackpad / no-middle-button behavior needs an explicit profile defined before Phase 1 ships.
- **Rollback suppress-below fidelity:** The `Body.Tip` engine exists but its exact interaction with `execute()` ordering and mid-tree insert needs a targeted code-read spike before Phase 3 behavioral specs are written.
- **`SoFCUnifiedSelection` box-select event routing:** Needs a targeted read before Phase 5 specs to understand what the delta to SW box-select directionality requires.
- **SW-user parity testing track:** "Zero relearning" can only be validated by daily SolidWorks users. This must be established in Phase 0 and run every phase.

---

## Sources

### Primary (HIGH confidence)

- FreeCAD `main` checkout @ `768e237091` — direct file reads: `src/Gui/Navigation/SolidWorksNavigationStyle.cpp`, `OverlayManager.{h,cpp}`, `OverlayWidgets.{h,cpp}`, `WorkbenchManipulator.h`, `ToolBarAreaWidget.h`, `ToolBarManager.h`, `Tree.h`, `Control.h`, `ComboView.h`, `MainWindow.cpp`, `BitmapFactory.h`, `StyleParameters/`, `Stylesheets/FreeCAD.qss`, `Workbench.{h,cpp}`, `DockWindowManager.h`, `Selection/Selection.h`, `BoxSelection.cpp`, `SelectionFilter.cpp`, `SoFCUnifiedSelection.cpp`, `src/Mod/PartDesign/App/Body.h`, `src/Mod/PartDesign/Gui/ViewProviderBody.cpp`, `TaskFeatureParameters.cpp`
- `.planning/PROJECT.md`, `.planning/codebase/{STACK,ARCHITECTURE,STRUCTURE,CONVENTIONS,CONCERNS,INTEGRATIONS}.md`
- https://github.com/czyt1988/SARibbon — MIT license, Qt 6.8 LTS support, v2.8.0 (May 2026), cross-platform
- SOLIDWORKS Help (official) — Mouse Gestures, CommandManager, PropertyManager
- TriMech, HawkRidge Systems, Innova Systems, Open WA Pressbooks — SW UI anatomy and feature set

### Secondary (MEDIUM confidence)

- Google v. Oracle (EFF/Copyright Lately) — reimplementation and functional-interface fair use context
- Apple v. Samsung (Stroock / Law Journal Newsletters) — functional UI layout and trade dress framework
- Turley Law — trade dress for digital products
- KDAB — "Say No to Qt Style Sheets" — cross-platform QSS metric side-effects, macOS pitfalls
- Qt Style Sheets documentation — QSS scope and native-style interaction

### Tertiary (for implementation guidance)

- GitHub Blog — Strategies for friendly fork management
- Open Energy Transition — Soft Fork Strategy (mirror-branch + patch-series)
- conda-forge — keep your fork in sync (range-diff, --skip patterns)

---
*Research completed: 2026-06-06*
*Ready for roadmap: yes*
