# Phase 2: CommandManager Ribbon - Context

**Gathered:** 2026-06-07
**Status:** Ready for planning

<domain>
## Phase Boundary

Deliver a SolidWorks-style tabbed **CommandManager ribbon** mounted in the top toolbar area that Phase 1's `FwLayout` already reserves in FreeWorks mode. The ribbon shows context-switching tabs (Features / Sketch / Evaluate / …) of large labeled icons with flyout split-buttons, is driven entirely by FreeCAD's existing command registry (`ToolBarItem`/`MenuItem` trees consumed as data, firing via `CommandManager` — no duplicated command backend), and persists its layout across restarts. It is built as a self-contained, merge-safe widget under `src/Gui/FreeWorks/` so an upstream sync never conflicts.

**In scope:** RIBBON-01 (tabbed ribbon, large labeled icons, flyout split-buttons, registry-driven), RIBBON-02 (context-driven active-tab switching). Building the ribbon shell widget, the curated tab/command map for the core modeling loop, the auto-derive fallback for uncurated workbenches, sketch-edit contextual tab switching, hiding FreeCAD's menu bar + toolbars in FreeWorks mode, and layout persistence.

**Out of scope (own phases):** FeatureManager tree (Phase 3), PropertyManager (Phase 4), selection/on-canvas accelerators (Phase 5), gestures/Instant3D (Phase 6), final pixel-level visual theme — colors/fonts/blue accent — and Task Pane + legal sign-off (Phase 7). Broader SW contextual-tab behavior beyond sketch-edit, ribbon galleries, the application button, collapse-to-tab/minimize, and user customization of the ribbon are deferred.
</domain>

<decisions>
## Implementation Decisions

### Ribbon Engine
- **D-01:** **Native-first.** Build the ribbon shell natively on Qt (`QTabWidget` + `QToolBar` hosted on the existing `Gui::ToolBarAreaWidget` / `TopToolBarArea`), per the research verdict — zero new deps, best upstream-merge story.
- **D-02:** **Validate with a time-boxed spike before committing the full build.** The spike proves whether native Qt reaches SW parity at the bar in D-03.
- **D-03:** **Spike pass/fail bar = core ribbon mechanics:** tabs + panel groups + large labeled icons + working flyout split-buttons + context tab-switch, looking convincingly SW-like. Galleries, application button, and collapse-to-tab/minimize are explicitly NOT part of the bar (deferred polish).
- **D-04:** **SARibbon is the pre-blessed fallback.** If the native spike misses the D-03 bar, adopt **SARibbon (MIT, v2.8.0, Qt 6.8)** vendored as a git submodule under `src/3rdParty/SARibbon` (CMake `add_subdirectory`) — no second decision round needed. Vendor it (not a Pixi/Conda dep) to keep tri-OS builds reproducible.

### Tabs & Command Source
- **D-05:** **Curated SW-faithful declarative map.** Author a data-driven map (tab → panels → FreeCAD command IDs) reproducing SolidWorks' tab/command organization, pulling icons/actions from the existing command registry. The map is a self-contained data file under `src/Gui/FreeWorks/` (merge-safe; no command backend duplicated).
- **D-06:** **Coverage = the core modeling loop.** Curate the PartDesign + Sketcher workflow tabs (Features, Sketch, Evaluate, optionally Surfaces) — the sketch→feature loop a SW user exercises first. Broader rosters (Sheet Metal, Weldments, assembly-equivalents) are deferred.
- **D-07:** **Uncurated workbenches auto-derive.** When the active workbench has no curated map (e.g. Mesh, CAM), build its ribbon tabs from that workbench's existing `ToolBarItem` groups so the ribbon is never empty and FreeWorks stays usable everywhere (effectively hybrid: curate where it counts, auto-derive elsewhere).
- **D-08:** **Gap policy = omit-missing, no-extras.** A SW command with no FreeCAD equivalent is silently skipped (only real command IDs are placed); FreeCAD-only commands are NOT auto-appended to curated tabs. Track notable gaps as notes for future phases. (Auto-derived tabs naturally show whatever the workbench exposes — this policy governs the *curated* maps.)

### Contextual Switching (RIBBON-02)
- **D-09:** **Sketch-edit drives v1 switching.** Entering sketch-edit auto-activates the Sketch tab; leaving restores the previously active tab. Wired to `Gui::Control` active-dialog / edit state (Success Criterion 4). This is the single context switch a SW user notices most; broader contextual tabs are deferred.
- **D-10:** **Context wins, then restore.** Entering a sketch always jumps to the Sketch tab even over a manual selection; on exit, restore whatever tab was active before — matches SW behavior and reverts cleanly.

### Chrome & Buttons
- **D-11:** **Full ribbon-only chrome.** In FreeWorks mode, hide FreeCAD's menu bar **and** redundant toolbars so the ribbon is the single command surface (Success Criterion 1, "replacing"). Must be reversible / scoped to FreeWorks mode (do not destroy stock FreeCAD chrome for other workbenches).
- **D-12:** **Discoverability escape hatch required (consequence of D-11 + D-06).** Because the menu bar is hidden but only the core loop is curated, planning MUST include a way to reach commands not on a curated tab — at minimum keep FreeCAD keyboard shortcuts working; ideally a command-search or "more commands" affordance. Do not strand commands. This closes the gap D-11 opens; it is not a new capability.
- **D-13:** **Button fidelity = structure + behavior, not final pixels.** Large icon-over-label primary buttons and working flyout split-buttons (e.g. Fillet ▸ Chamfer) driven by the registry's command groups (Success Criterion 3), using existing icons. Exact SW theming (colors, fonts, blue accent, precise sizing) is Phase 7's job.

### Persistence
- **D-14:** **Layout persists across restarts** (Success Criterion 5) — tab state and panel positions restore correctly, reusing FreeCAD's `DockWindowManager` / toolbar-area state save-restore where possible. v1 ribbon layout is fixed/curated (not user-customizable); customization is deferred.

### Claude's Discretion — delegated to research/planning, confirm at plan review
- Exact spike time-box duration and the concrete parity checklist used to judge D-03.
- The curated-map file format (e.g. JSON/structured C++ table) and exact tab/panel/command-ID contents for the core loop — verify against version-pinned SOLIDWORKS Help (the SW2024/2025 default tab roster and command grouping).
- Whether contextual switching reads `Control` edit-state via signal/observer vs polling, and the precise FreeCAD hook for "entered/left sketch edit."
- The exact discoverability escape-hatch mechanism for D-12 (shortcuts-only vs command-search vs overflow menu).
- Whether the ribbon mounts via `WorkbenchManipulator` vs direct `FwWorkbench`/`FwLayout` install — pick the most additive/merge-safe seam.
</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase scope & requirements
- `.planning/ROADMAP.md` § Phase 2 — goal, mode (mvp), 5 success criteria
- `.planning/REQUIREMENTS.md` — RIBBON-01, RIBBON-02 (exact behavior wording)
- `.planning/PROJECT.md` — product vision, constraints (App/Gui separation, upstream-tracking, no proprietary assets)

### Carried-forward foundation (Phase 1 — locked, MUST honor)
- `.planning/phases/01-solidworks-mode-foundation/01-CONTEXT.md` — D-02 `Fw` naming (class `FwRibbon`, namespace `FreeWorksGui`, dir `src/Gui/FreeWorks/`), D-03 no-"SolidWorks"-identifiers rule, observe-the-DOM, additive-module discipline
- `.planning/phases/01-solidworks-mode-foundation/01-01-SUMMARY.md` — `FwWorkbench` activation seam, `FwLayout` dock-shell installer (top ribbon area reserved), permanent `Fw_*` dock names, `target_sources()` build pattern, single `// SW-FORK HOOK` shared edit
- `src/Gui/FreeWorks/` — existing module: `FwWorkbench`, `FwLayout`, `FwTheme`, `FwNavigationDefault`, `CMakeLists.txt`, `InitGui.py`, `Resources/FreeWorks.qrc`

### Architecture & integration (the HOW)
- `.planning/research/STACK.md` § "How the pieces map to SolidWorks concepts" + ribbon row — native `ToolBarManager`/`ToolBarAreaWidget`/`QTabWidget` first, SARibbon (MIT v2.8.0) fallback + vendoring recipe; `WorkbenchManipulator` extension point
- `.planning/research/ARCHITECTURE.md` — additive module, hook points, build order, upstream-mergeability strategy, anti-patterns (apply `Fw` naming over `Sw` examples)
- `.planning/research/SUMMARY.md` — cross-cutting findings, suggested build order
- `.planning/research/questions.md` § "Ribbon: native vs SARibbon" — the spike this phase resolves (D-01..D-04)
- `.planning/codebase/STRUCTURE.md`, `.planning/codebase/CONVENTIONS.md` — where Gui code lives, CMake + registration patterns
- `.planning/codebase/ARCHITECTURE.md` — App/Gui separation, anti-patterns
- `.planning/codebase/TESTING.md` — GTest/ctest verification patterns (mirror Phase 1's headless `Gui_tests_run` approach)

### Risk & legal
- `.planning/research/PITFALLS.md` — upstream-merge discipline (`// SW-FORK HOOK`), asset-provenance for any new ribbon icons, App/Gui-contract breakage
- `.planning/research/SOLIDWORKS-UI.md` — verified SW UI behavior to match; verify version-pinned SW Help for the exact default tab roster before authoring the curated map
- `ASSET_PROVENANCE.md` + `tools/fw-provenance-guard.sh` + `tools/fw-string-leak-grep.sh` — CI gates any new icon and any "SolidWorks" string must satisfy
</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- **`Gui::ToolBarAreaWidget` / `ToolBarArea::TopToolBarArea`** (`src/Gui/ToolBarAreaWidget.h`): the reserved mount point for the ribbon (Phase 1 left this area for Phase 2).
- **`Gui::ToolBarManager` / `ToolBarItem` / `MenuItem`** : convert `CommandManager` commands → widgets; consume these trees as data to build ribbon panels (RIBBON-01 / SC2 — no duplicated backend).
- **`Gui::CommandManager`** (`src/Gui/CommandManager.h`): commands fire through here — ribbon buttons are thin triggers.
- **`Gui::WorkbenchManipulator`** (added 2023): additive seam to inject/override toolbars/menus per workbench — candidate for mounting the ribbon and hiding stock chrome with minimal shared-file edits.
- **`FwWorkbench` / `FwLayout`** (`src/Gui/FreeWorks/`): the activation seam — `activated()` → `FwLayout::install`; extend to build + mount `FwRibbon` and hide menu bar/toolbars.
- **`Gui::DockWindowManager`** + `QMainWindow` toolbar-area state: layout persistence (SC5) largely free via existing save/restore.
- **`BitmapFactory::pixmapFromSvg()`** + `.qrc`: icon rendering for any new SVGs (subject to `ASSET_PROVENANCE.md` + provenance guard).

### Established Patterns (must follow)
- **Additive module + activation seam**: all ribbon code under `src/Gui/FreeWorks/`; only shared edits are `src/Gui/CMakeLists.txt` (already wired) and any unavoidable touch marked `// SW-FORK HOOK`. If SARibbon is adopted, add it under `src/3rdParty/SARibbon` as a submodule + one `add_subdirectory`.
- **Observe-the-DOM, never mutate directly**: read command/registry/edit-state via getters/signals; trigger via `Gui::Command`/`CommandManager`. No App-layer changes.
- **No "SolidWorks" identifiers/strings** in new code (leak grep CI gate); descriptive behavior comments are fine.
- **Verification mirrors Phase 1**: headless GTest in `Gui_tests_run` (ribbon builds from a known command tree, context switch toggles tab, persistence round-trips) + manual tri-OS checklist where GUI-only.

### Integration Points
- `src/Gui/FreeWorks/CMakeLists.txt` — add new ribbon sources via `target_sources()`.
- `FwWorkbench::activated()` / `FwLayout::install()` — mount `FwRibbon` in `TopToolBarArea`; hide stock menu bar + toolbars (FreeWorks-scoped, reversible).
- `Gui::Control` active-dialog / edit-state — source for sketch-edit contextual tab switching (RIBBON-02 / SC4).
- Command registry (`ToolBarItem` trees per workbench) — data source for both curated maps and the auto-derive fallback.
</code_context>

<specifics>
## Specific Ideas

- Class name verbatim: **`FwRibbon`**; namespace **`FreeWorksGui`**; all code under **`src/Gui/FreeWorks/`**.
- SARibbon fallback path verbatim: **`src/3rdParty/SARibbon`** (git submodule, MIT v2.8.0), CMake `add_subdirectory` — only if the native spike misses the D-03 bar.
- The curated tab/command map is a **declarative data file** (format TBD at planning), not hard-coded widget construction — keeps it merge-safe and easy to extend.
- "Full ribbon-only" = hide BOTH menu bar and toolbars, but FreeWorks-scoped and reversible (don't break stock FreeCAD for other modes).
</specifics>

<deferred>
## Deferred Ideas

- **Broader SW contextual tabs** — auto-switch/appear based on selection type and document context beyond sketch-edit. (Later phase; v1 is sketch-edit only.)
- **Ribbon galleries, application button, collapse-to-tab/minimize** — SW polish features outside the spike's parity bar. (Likely Phase 7 / later.)
- **User customization of the ribbon** — add/remove/reorder commands (SW's CommandManager customize dialog). v1 layout is fixed/curated.
- **Full pixel-level SW theming** of ribbon buttons (colors, fonts, blue accent, exact sizing) — Phase 7 (Visual Theme).
- **Curated tab rosters for non-core workbenches** (Sheet Metal, Weldments, assembly-equivalents) — beyond the core modeling loop; auto-derive fallback covers them functionally for now.
</deferred>

---

*Phase: 2-CommandManager Ribbon*
*Context gathered: 2026-06-07*
