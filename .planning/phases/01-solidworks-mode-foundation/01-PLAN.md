---
phase: 01-solidworks-mode-foundation
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - src/Gui/FreeWorks/CMakeLists.txt
  - src/Gui/FreeWorks/PreCompiled.h
  - src/Gui/FreeWorks/PreCompiled.cpp
  - src/Gui/FreeWorks/FwWorkbench.h
  - src/Gui/FreeWorks/FwWorkbench.cpp
  - src/Gui/FreeWorks/FwLayout.h
  - src/Gui/FreeWorks/FwLayout.cpp
  - src/Gui/FreeWorks/FwTheme.h
  - src/Gui/FreeWorks/FwTheme.cpp
  - src/Gui/FreeWorks/InitGui.py
  - src/Gui/FreeWorks/Resources/FreeWorks.qrc
  - src/Gui/CMakeLists.txt
autonomous: true
requirements: [SHELL-01]
must_haves:
  truths:
    - "User can select 'FreeWorks' in the FreeCAD workbench selector"
    - "Activating FreeWorks mounts a coherent SolidWorks-style dock shell with Fw_* placeholder docks"
    - "Fw_FeatureManager and Fw_PropertyManager dock on the left; Fw_TaskPane reserved on the right; top toolbar area left free for the future ribbon"
    - "Zero edits to MainWindow.cpp bodies; the only shared-file touch is one marked line in src/Gui/CMakeLists.txt"
  artifacts:
    - path: "src/Gui/FreeWorks/FwWorkbench.h"
      provides: "FwWorkbench class declaration (subclass of Gui::StdWorkbench, namespace FreeWorksGui)"
      contains: "class FwWorkbench"
      min_lines: 20
    - path: "src/Gui/FreeWorks/FwWorkbench.cpp"
      provides: "setupDockWindows() registering Fw_* dock names + activated() calling FwLayout::install()"
      contains: "Fw_FeatureManager"
    - path: "src/Gui/FreeWorks/FwLayout.cpp"
      provides: "FwLayout::install() mounting labeled placeholder widgets under Fw_* dock names"
      contains: "registerDockWindow"
    - path: "src/Gui/FreeWorks/CMakeLists.txt"
      provides: "FreeWorks module build target linked into FreeCADGui"
    - path: "src/Gui/CMakeLists.txt"
      provides: "add_subdirectory(FreeWorks) marked with // SW-FORK HOOK"
      contains: "add_subdirectory(FreeWorks)"
  key_links:
    - from: "src/Gui/FreeWorks/InitGui.py"
      to: "FreeWorksGui::FwWorkbench"
      via: "GetClassName() return + Gui.addWorkbench"
      pattern: "FreeWorksGui::FwWorkbench"
    - from: "src/Gui/FreeWorks/FwWorkbench.cpp"
      to: "FwLayout::install"
      via: "activated() override"
      pattern: "FwLayout::install"
    - from: "src/Gui/CMakeLists.txt"
      to: "src/Gui/FreeWorks/CMakeLists.txt"
      via: "add_subdirectory(FreeWorks)"
      pattern: "add_subdirectory\\(FreeWorks\\)"
---

<objective>
Build the Walking Skeleton: a new additive `src/Gui/FreeWorks/` Gui submodule that compiles and links into FreeCADGui, registers an `FwWorkbench` (label "FreeWorks", namespace `FreeWorksGui`, `Fw` prefix) via the standard Workbench plugin path, and on activation mounts a coherent SolidWorks-style dock shell of labeled placeholder `Fw_*` docks — proving the additive module compiles, links, registers, activates, and installs the shell end-to-end with zero `MainWindow.cpp` body edits.

Purpose: This is the mergeability backbone and the entry seam onto which every later phase (ribbon, tree, PropertyManager) mounts. It satisfies SHELL-01 (launch into a SolidWorks-mode coherent layout).
Output: The `src/Gui/FreeWorks/` module, the `FwWorkbench`/`FwLayout`/`FwTheme` classes, the `Fw_*` dock names, the InitGui.py registration, and the single marked `add_subdirectory(FreeWorks)` edit in `src/Gui/CMakeLists.txt`.
</objective>

<execution_context>
@$HOME/.claude/gsd-core/workflows/execute-plan.md
@$HOME/.claude/gsd-core/templates/summary.md
</execution_context>

<context>
@.planning/PROJECT.md
@.planning/ROADMAP.md
@.planning/STATE.md
@.planning/phases/01-solidworks-mode-foundation/01-SKELETON.md
@.planning/phases/01-solidworks-mode-foundation/01-PATTERNS.md
@.planning/phases/01-solidworks-mode-foundation/01-CONTEXT.md
</context>

<artifacts_produced>
## Artifacts this phase produces (Plan 01)

| Symbol / Path | Kind |
|---|---|
| `namespace FreeWorksGui` | C++ namespace (D-02) |
| `FreeWorksGui::FwWorkbench` | class (subclass of `Gui::StdWorkbench`) |
| `FreeWorksGui::FwLayout` | class (dock-shell installer) |
| `FreeWorksGui::FwTheme` | class (minimal QSS/theme hook, reserved for Phase 7) |
| `Fw_FeatureManager`, `Fw_PropertyManager`, `Fw_TaskPane` | permanent dock names (OQ-2) |
| `src/Gui/FreeWorks/` | new additive module directory |
| `src/Gui/FreeWorks/{CMakeLists.txt, PreCompiled.{h,cpp}, FwWorkbench.{h,cpp}, FwLayout.{h,cpp}, FwTheme.{h,cpp}, InitGui.py, Resources/FreeWorks.qrc}` | files |
| `add_subdirectory(FreeWorks)  # SW-FORK HOOK` in `src/Gui/CMakeLists.txt` | shared-file edit #1 of 2 |
</artifacts_produced>

<tasks>

<task type="auto">
  <name>Task 1: Scaffold the FreeWorks Gui submodule (build wiring + PreCompiled + theme hook)</name>
  <read_first>
    - 01-PATTERNS.md section "PreCompiled" (analog: any module `PreCompiled.{h,cpp}`, e.g. `src/Mod/Part/Gui/PreCompiled.h`) — copy include-guard + common-header block, adapt export macro to the FreeWorks module.
    - 01-PATTERNS.md section "CMakeLists.txt + Resources" (analog: `src/Gui/CMakeLists.txt` `qt_add_resources` at :447 and the `SET(..._SRCS)` + `add_library`/`target_link_libraries` pattern).
    - 01-PATTERNS.md section "src/Gui/CMakeLists.txt (EDIT)" (analog: existing `add_subdirectory(...)` at `src/Gui/CMakeLists.txt:14-16`).
    - 01-PATTERNS.md section "FwTheme" (analog: `tests/src/Gui/StyleParameters/`) — minimal hook only; full QSS deferred to Phase 7.
    - 01-CONTEXT.md D-02 (naming: `Fw` / `FreeWorksGui` / `src/Gui/FreeWorks/`) and Shared Patterns SPDX header.
  </read_first>
  <action>
    Create `src/Gui/FreeWorks/PreCompiled.{h,cpp}` by adapting an existing module's PreCompiled block, defining the FreeWorks export macro (e.g. `FreeWorksGuiExport`). Create `src/Gui/FreeWorks/FwTheme.{h,cpp}` as a minimal class in `namespace FreeWorksGui` exposing a static `apply()` no-op/stub hook (full QSS/SVG theme is Phase 7 — do not implement it here). Create `src/Gui/FreeWorks/CMakeLists.txt` modeled on the Gui SET-source + `add_library`/`target_link_libraries` pattern, listing all module sources, linking `FreeCADGui`, and wiring `Resources/FreeWorks.qrc` via `qt_add_resources`. Create `src/Gui/FreeWorks/Resources/FreeWorks.qrc` referencing a single recreated-original workbench icon placeholder (the icon binary and its ASSET_PROVENANCE.md row are handled in Plan 04 — reference the path; do not embed a copied proprietary asset). Add exactly one marked line `add_subdirectory(FreeWorks)  # SW-FORK HOOK` to `src/Gui/CMakeLists.txt` in the existing add_subdirectory block. Every new file starts with the SPDX header on line 1 (`// SPDX-License-Identifier: LGPL-2.1-or-later`, `#` form for CMake/qrc). NO App-layer includes anywhere in this module.
  </action>
  <verify>
    <automated>test -f src/Gui/FreeWorks/CMakeLists.txt &amp;&amp; test -f src/Gui/FreeWorks/PreCompiled.h &amp;&amp; test -f src/Gui/FreeWorks/FwTheme.h &amp;&amp; test "$(grep -c 'add_subdirectory(FreeWorks)' src/Gui/CMakeLists.txt)" = "1"</automated>
  </verify>
  <acceptance_criteria>
    - `src/Gui/FreeWorks/{CMakeLists.txt,PreCompiled.{h,cpp},FwTheme.{h,cpp},Resources/FreeWorks.qrc}` exist.
    - `src/Gui/CMakeLists.txt` contains exactly one `add_subdirectory(FreeWorks)` line marked `# SW-FORK HOOK`.
    - No `include <App/` appears under `src/Gui/FreeWorks/`.
  </acceptance_criteria>
  <done>The FreeWorks module skeleton exists and is wired into the Gui build via a single marked add_subdirectory line; no App-layer include leaks into the module.</done>
</task>

<task type="auto">
  <name>Task 2: Implement FwWorkbench (StdWorkbench subclass) + Fw_* dock registration</name>
  <read_first>
    - 01-PATTERNS.md section "FwWorkbench" — header shape from `src/Mod/Part/Gui/Workbench.h:1-57`; override signatures from `src/Gui/Workbench.h:116-123,164-205`; `TYPESYSTEM_SOURCE` from `src/Gui/Workbench.cpp:623`; `setupDockWindows()` adapted from `Gui::StdWorkbench::setupDockWindows()` `src/Gui/Workbench.cpp:918-943`; `activated()` modeled on `Gui::BlankWorkbench::activated()` `src/Gui/Workbench.cpp:955-962`.
    - 01-PATTERNS.md Shared Patterns: TYPESYSTEM registration and SPDX header.
    - 01-CONTEXT.md D-01 (label "FreeWorks"), D-02 (naming), D-03 (no "SolidWorks" identifiers in new code).
  </read_first>
  <action>
    Create `src/Gui/FreeWorks/FwWorkbench.{h,cpp}` declaring `class FwWorkbench : public Gui::StdWorkbench` in `namespace FreeWorksGui`, with `TYPESYSTEM_HEADER_WITH_OVERRIDE()` in the header and `TYPESYSTEM_SOURCE(FreeWorksGui::FwWorkbench, Gui::StdWorkbench)` in the cpp. Subclass `StdWorkbench` (not bare `Workbench`) so the future ribbon inherits the Std command set. Override `setupDockWindows()` to register the PERMANENT dock names `Fw_FeatureManager` (LeftDockWidgetArea, Visible), `Fw_PropertyManager` (LeftDockWidgetArea, Visible), and reserve `Fw_TaskPane` (RightDockWidgetArea, VisibleTabbed); leave the TOP toolbar area free for the Phase 2 ribbon. Do NOT touch the upstream `Std_*` dock blocks. Override `activated()` to call `FwLayout::install(Gui::getMainWindow())` then `StdWorkbench::activated()`. Override `setupMenuBar()`/`setupToolBars()`/`setupCommandBars()` to defer to the StdWorkbench implementations (placeholder for Phase 2). No "SolidWorks" string appears anywhere in this file (D-03).
  </action>
  <verify>
    <automated>grep -q "class FwWorkbench" src/Gui/FreeWorks/FwWorkbench.h &amp;&amp; grep -q "namespace FreeWorksGui" src/Gui/FreeWorks/FwWorkbench.h &amp;&amp; grep -q "Gui::StdWorkbench" src/Gui/FreeWorks/FwWorkbench.h &amp;&amp; grep -q "Fw_FeatureManager" src/Gui/FreeWorks/FwWorkbench.cpp &amp;&amp; grep -q "FwLayout::install" src/Gui/FreeWorks/FwWorkbench.cpp &amp;&amp; ! grep -i "SolidWorks" src/Gui/FreeWorks/FwWorkbench.cpp src/Gui/FreeWorks/FwWorkbench.h</automated>
  </verify>
  <acceptance_criteria>
    - `src/Gui/FreeWorks/FwWorkbench.h` contains `class FwWorkbench` and `namespace FreeWorksGui`, subclassing `Gui::StdWorkbench`.
    - `setupDockWindows()` registers `Fw_FeatureManager`, `Fw_PropertyManager`, `Fw_TaskPane`; no `Std_*` dock block is modified.
    - `activated()` calls `FwLayout::install(...)`.
    - Zero occurrences of the string "SolidWorks" in FwWorkbench.{h,cpp}.
  </acceptance_criteria>
  <done>FwWorkbench compiles as a TYPESYSTEM-registered StdWorkbench subclass that registers the three Fw_* dock names and installs FwLayout on activation, with no upstream dock-block edits and no "SolidWorks" identifier.</done>
</task>

<task type="auto">
  <name>Task 3: Implement FwLayout (placeholder dock-shell installer) + register the workbench</name>
  <read_first>
    - 01-PATTERNS.md section "FwLayout" — `getMainWindow()` + `findChildren<QDockWidget*>()` access pattern from `Gui::BlankWorkbench::activated()` `src/Gui/Workbench.cpp:955-962`; placeholder-widget registration via `DockWindowManager::registerDockWindow(const char*, QWidget*)` `src/Gui/DockWindowManager.h:84`.
    - 01-PATTERNS.md section "Workbench registration" — `src/Mod/Part/InitGui.py:32-75` (`Gui.addWorkbench`, `GetClassName()` returns `FreeWorksGui::FwWorkbench`, `MenuText`/`ToolTip` = "FreeWorks").
    - 01-PATTERNS.md Shared Patterns "Observe-the-DOM" — read via `getMainWindow()`/getters; never edit `MainWindow.cpp`.
    - 01-CONTEXT.md D-01/D-02/D-03.
  </read_first>
  <action>
    Create `src/Gui/FreeWorks/FwLayout.{h,cpp}` declaring `class FwLayout` in `namespace FreeWorksGui` with `static void install(QMainWindow* mainWindow)`. `install()` backs each permanent dock name with a labeled placeholder `QWidget` (a simple `QLabel` reading e.g. "FeatureManager (Phase 3)", "PropertyManager (Phase 4)", "Task Pane (Phase 7)") registered via `Gui::DockWindowManager::instance()->registerDockWindow("Fw_FeatureManager", widget)` and the same for `Fw_PropertyManager` and `Fw_TaskPane`, then arranges the docks into the coherent SolidWorks left/right geometry by reading `getMainWindow()` and using public Qt dock APIs only. FwLayout MUST NOT include or edit `MainWindow.cpp` and MUST NOT include any App-layer header (observe-the-DOM). Create `src/Gui/FreeWorks/InitGui.py`: a `FreeWorksWorkbench(Gui.Workbench)` whose `MenuText`/`ToolTip` are "FreeWorks" (D-01; never "SolidWorks" — D-03), whose `Icon` points at the Resources icon path, whose `GetClassName()` returns the string `"FreeWorksGui::FwWorkbench"`, registered via `Gui.addWorkbench(FreeWorksWorkbench())`. SPDX header on line 1 of every new file.
  </action>
  <verify>
    <automated>grep -q "registerDockWindow" src/Gui/FreeWorks/FwLayout.cpp &amp;&amp; grep -q "Fw_FeatureManager" src/Gui/FreeWorks/FwLayout.cpp &amp;&amp; grep -q "FreeWorksGui::FwWorkbench" src/Gui/FreeWorks/InitGui.py &amp;&amp; grep -q "addWorkbench" src/Gui/FreeWorks/InitGui.py &amp;&amp; ! grep -q "include <App/" src/Gui/FreeWorks/FwLayout.cpp</automated>
  </verify>
  <acceptance_criteria>
    - `FwLayout::install()` registers labeled placeholder widgets under `Fw_FeatureManager`, `Fw_PropertyManager`, `Fw_TaskPane` via `registerDockWindow`.
    - `src/Gui/FreeWorks/FwLayout.cpp` contains no `include <App/` (no App-layer dependency).
    - `InitGui.py` returns `"FreeWorksGui::FwWorkbench"` from `GetClassName()` and calls `Gui.addWorkbench(...)` with `MenuText`/`ToolTip` = "FreeWorks".
    - After build, launching FreeCAD shows "FreeWorks" in the workbench selector and activating it mounts the three Fw_* docks (verified in Plan 03 manual tri-OS checklist + Plan 03 GTest registration assertion).
  </acceptance_criteria>
  <done>Activating FreeWorks registers and mounts the three labeled Fw_* placeholder docks in SW geometry, the workbench appears in the selector via InitGui.py, and the module has zero App-layer includes.</done>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| upstream FreeCAD main -> fork | Moving-target shared code; unmarked edits cause merge drift |
| recreated assets -> repo | Workbench icon must be provenance-clean (no proprietary SW asset) |

## STRIDE Threat Register

| Threat ID | Category | Component | Disposition | Mitigation Plan |
|-----------|----------|-----------|-------------|-----------------|
| T-01-01 | Tampering | `src/Gui/CMakeLists.txt` shared edit | mitigate | Single `add_subdirectory(FreeWorks)` line marked `# SW-FORK HOOK`; verified greppable (== 1) in this plan and by Plan 04 sync drill |
| T-01-02 | Information Disclosure | App/Gui separation | mitigate | Module has zero App-layer includes; FwLayout observes via `getMainWindow()` getters only; headless gate (Plan 03) is the detector |
| T-01-03 | Spoofing/Legal | workbench icon asset | accept (here) / mitigate (Plan 04) | Icon is recreated-original; provenance row + CI guard enforced in Plan 04, not at scaffold time |
| T-01-SC | Tampering | npm/pip/cargo installs | n/a | No package-manager installs in this phase (native Qt6 Widgets only — SKELETON Build/CI row); no legitimacy gate required |
</threat_model>

<verification>
- `src/Gui/FreeWorks/` module compiles and links into FreeCADGui (proven by Plan 03 CI matrix build).
- Exactly one shared-file edit in this plan: `grep -c 'add_subdirectory(FreeWorks)' src/Gui/CMakeLists.txt == 1`, marked `# SW-FORK HOOK`.
- No "SolidWorks" identifier in any new FreeWorks source (enforced by Plan 04 leak grep).
- No App-layer include under `src/Gui/FreeWorks/` (enforced by Plan 03 headless gate).
</verification>

<success_criteria>
- "FreeWorks" appears in the FreeCAD workbench selector.
- Activating FreeWorks mounts `Fw_FeatureManager` + `Fw_PropertyManager` (left) and reserves `Fw_TaskPane` (right); top toolbar area free.
- Zero `MainWindow.cpp` body edits; one marked CMake edit.
</success_criteria>

<output>
Create `.planning/phases/01-solidworks-mode-foundation/01-01-SUMMARY.md` when done.
</output>
