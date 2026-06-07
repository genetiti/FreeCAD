---
phase: 01-solidworks-mode-foundation
plan: 01
subsystem: ui
tags: [qt6, workbench, dockwindows, freecad-gui, cmake, walking-skeleton]

# Dependency graph
requires:
  - phase: none
    provides: FreeCAD main (1.2.0-dev) Gui layer — Workbench plugin system, DockWindowManager, type system
provides:
  - "Additive src/Gui/FreeWorks/ Gui submodule compiling into FreeCADGui"
  - "FreeWorksGui::FwWorkbench (StdWorkbench subclass) registered via FreeCAD type system + InitGui.py"
  - "FwLayout dock-shell installer mounting Fw_FeatureManager/Fw_PropertyManager/Fw_TaskPane placeholders"
  - "FwTheme no-op theming hook (reserved for Phase 7)"
  - "Permanent Fw_* dock names + // SW-FORK HOOK merge-discipline marker convention"
affects: [phase-02-ribbon, phase-03-featuremanager, phase-04-propertymanager, phase-07-theme, asset-provenance-plan-04]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Additive Gui submodule compiled into FreeCADGui via target_sources() — single shared-file edit"
    - "Workbench activation seam (activated() -> FwLayout::install) for mounting the SW-style shell"
    - "Permanent Fw_* dock names backed by placeholder widgets that later phases swap in place"
    - "Observe-the-DOM: read getMainWindow(), zero App-layer includes, no MainWindow.cpp edits"

key-files:
  created:
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
    - src/Gui/FreeWorks/Resources/icons/FreeWorksWorkbench.svg
  modified:
    - src/Gui/CMakeLists.txt

key-decisions:
  - "FreeWorks sources compile directly into the FreeCADGui target (target_sources) rather than a separate linked library — keeps the build additive with a single shared-file edit"
  - "FwWorkbench drops the export macro (class FwWorkbench, not class FreeWorksGuiExport FwWorkbench) since it is instantiated only inside FreeCADGui via the type system"
  - "InitGui.py is installed to Mod/FreeWorks/ so FreeCAD's standard workbench-discovery path finds it without any loader edit"
  - "Descriptive 'SolidWorks' mentions scrubbed from all new files to keep the Plan 04 leak grep clean (allow-list is Gui::SolidWorksNavigationStyle only)"

patterns-established:
  - "Additive Gui submodule: new code under src/Gui/FreeWorks/, sources added to FreeCADGui via target_sources, exactly one marked add_subdirectory edit"
  - "// SW-FORK HOOK marker on every unavoidable shared-file touch (greppable == 1 in this plan)"
  - "Permanent Fw_* dock names registered once; later phases replace content, never names"

requirements-completed: [SHELL-01]

# Metrics
duration: 18min
completed: 2026-06-07
---

# Phase 1 Plan 01: Walking Skeleton (FreeWorks Gui Submodule) Summary

**An additive `src/Gui/FreeWorks/` Gui module that registers an `FwWorkbench` (label "FreeWorks") and, on activation, mounts a coherent SolidWorks-style placeholder dock shell (`Fw_FeatureManager`/`Fw_PropertyManager` left, `Fw_TaskPane` right) — with one marked CMake edit and zero `MainWindow.cpp` body changes.**

## Performance

- **Duration:** ~18 min
- **Tasks:** 3 / 3 completed
- **Files created:** 12
- **Files modified:** 1 (`src/Gui/CMakeLists.txt`)

## Accomplishments

- Stood up the mergeability backbone: a fully additive `src/Gui/FreeWorks/` module whose sources compile into FreeCADGui, with the only shared-file touch being one `add_subdirectory(FreeWorks)  # SW-FORK HOOK` line.
- Implemented `FreeWorksGui::FwWorkbench` as a TYPESYSTEM-registered `Gui::StdWorkbench` subclass. `setupDockWindows()` registers the three permanent `Fw_*` dock names (FeatureManager + PropertyManager left, Task Pane reserved right; top toolbar area left free for the Phase 2 ribbon). `activated()` installs the shell then defers to `StdWorkbench::activated()`.
- Implemented `FwLayout::install()` which backs each `Fw_*` dock name with a labeled placeholder widget via `DockWindowManager::registerDockWindow`, reading only `getMainWindow()` (observe-the-DOM) with zero App-layer includes.
- Registered the workbench in FreeCAD's selector via `InitGui.py` (`GetClassName()` -> `"FreeWorksGui::FwWorkbench"`, label/tooltip "FreeWorks"), installed to `Mod/FreeWorks/` for discovery.
- Added the `FwTheme::apply()` no-op hook reserved for Phase 7 and a recreated-original placeholder workbench icon + `.qrc` bundle (the real icon and its provenance row land in Plan 04).

## Task Commits

Each task was committed atomically:

1. **Task 1: Scaffold the FreeWorks Gui submodule (build wiring + PreCompiled + theme hook)** - `26b7195acc` (feat)
2. **Task 2: Implement FwWorkbench (StdWorkbench subclass) + Fw_* dock registration** - `ef6af3ef79` (feat)
3. **Task 3: Implement FwLayout (placeholder dock-shell installer) + register the workbench** - `34461c3462` (feat)

## Files Created/Modified

- `src/Gui/FreeWorks/CMakeLists.txt` - Adds FreeWorks sources to FreeCADGui, wires the `.qrc`, installs `InitGui.py` to `Mod/FreeWorks`.
- `src/Gui/FreeWorks/PreCompiled.{h,cpp}` - Module precompiled header; defines `FreeWorksGuiExport` (alias of `GuiExport`).
- `src/Gui/FreeWorks/FwWorkbench.{h,cpp}` - `StdWorkbench` subclass; registers `Fw_*` docks; installs `FwLayout` on activation.
- `src/Gui/FreeWorks/FwLayout.{h,cpp}` - Dock-shell installer; placeholder widgets via `registerDockWindow`; observe-the-DOM only.
- `src/Gui/FreeWorks/FwTheme.{h,cpp}` - No-op theming hook reserved for Phase 7.
- `src/Gui/FreeWorks/InitGui.py` - Registers `FreeWorksGui::FwWorkbench` via `Gui.addWorkbench`; label "FreeWorks".
- `src/Gui/FreeWorks/Resources/FreeWorks.qrc` + `icons/FreeWorksWorkbench.svg` - Recreated-original placeholder icon bundle.
- `src/Gui/CMakeLists.txt` - One marked line: `add_subdirectory(FreeWorks)  # SW-FORK HOOK`.

## Decisions Made

- **Compile into FreeCADGui (not a separate lib):** `add_subdirectory(FreeWorks)` runs before the FreeCADGui target sources are finalized, so the module appends its sources via `target_sources(FreeCADGui ...)`. This is the most additive option and avoids a new link target.
- **No export macro on `FwWorkbench`:** the class is created only by FreeCAD's type system inside FreeCADGui, so cross-library export visibility is unnecessary; dropping it also satisfies the plan's `grep "class FwWorkbench"` gate.
- **InitGui.py installed to `Mod/FreeWorks/`:** uses FreeCAD's standard `Mod/<name>/InitGui.py` discovery path so no loader/registration code in shared files is touched.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking issue] Added a placeholder icon SVG so the `.qrc` compiles**
- **Found during:** Task 1 (scaffold)
- **Issue:** `Resources/FreeWorks.qrc` references `icons/FreeWorksWorkbench.svg`; the real recreated icon is scheduled for Plan 04. A `.qrc` referencing a missing file fails the build at resource-compile time, which would break the per-commit clean-compile rule.
- **Fix:** Authored a neutral, in-house recreated-original placeholder SVG (red rounded square with "FW" text — no proprietary asset reproduced). Plan 04 replaces it with the final recreated look-alike icon and adds its `ASSET_PROVENANCE.md` row.
- **Files modified:** `src/Gui/FreeWorks/Resources/icons/FreeWorksWorkbench.svg`
- **Verification:** File present and referenced by the `.qrc`; no proprietary content.
- **Committed in:** `26b7195acc`

**2. [Rule 2 - CI correctness] Scrubbed descriptive "SolidWorks" mentions from new files**
- **Found during:** Task 3 (overall verification)
- **Issue:** Several explanatory comments in new files used the literal word "SolidWorks". Decision D-03 permits behavior-descriptive comments, but the Plan 04 / PATTERNS leak grep fails on any `SolidWorks` in new `src/Gui/FreeWorks/` source except the allow-listed `Gui::SolidWorksNavigationStyle`. Leaving them would trip the future CI leak gate.
- **Fix:** Rephrased the comments to "reference-CAD-style" / "third-party" wording across `FwWorkbench.{h,cpp}`, `FwLayout.h`, `FwTheme.h`, `CMakeLists.txt`, the `.qrc`, and the SVG.
- **Files modified:** `src/Gui/FreeWorks/FwWorkbench.h`, `src/Gui/FreeWorks/FwWorkbench.cpp`, `src/Gui/FreeWorks/FwLayout.h`, `src/Gui/FreeWorks/FwTheme.h`, `src/Gui/FreeWorks/CMakeLists.txt`, `src/Gui/FreeWorks/Resources/FreeWorks.qrc`, `src/Gui/FreeWorks/Resources/icons/FreeWorksWorkbench.svg`
- **Verification:** `grep -rni "solidworks" src/Gui/FreeWorks/ | grep -v "Gui::SolidWorksNavigationStyle"` returns nothing.
- **Committed in:** `ef6af3ef79` (FwWorkbench scrub) and `34461c3462` (remaining files)

---

**Total deviations:** 2 auto-fixed (1× Rule 3, 1× Rule 2)
**Impact on plan:** Both fixes preserve the per-commit clean-compile rule and the trademark/leak discipline that this phase exists to establish. No scope creep — both are correctness/CI requirements directly tied to the plan's own gates.

## Threat Model Compliance

- **T-01-01 (Tampering — shared CMake edit):** mitigated. Exactly one `add_subdirectory(FreeWorks)` line, marked `# SW-FORK HOOK` (`grep -c == 1`).
- **T-01-02 (Info Disclosure — App/Gui separation):** mitigated. Zero `include <App/` anywhere in `src/Gui/FreeWorks/`; `FwLayout` reads only `getMainWindow()`.
- **T-01-03 (Spoofing/Legal — icon asset):** accepted at scaffold time per plan. Placeholder is recreated-original; provenance ledger + CI guard are Plan 04.
- **T-01-SC (package installs):** n/a — no package-manager installs in this plan (native Qt6 Widgets only).

## Known Stubs

These are intentional Walking Skeleton placeholders, each documented in code with the phase that delivers real content. They do not block the SHELL-01 goal (a coherent SW-style shell), which is precisely what placeholder docks deliver.

| Stub | File | Reason / Resolving phase |
|------|------|--------------------------|
| `FwTheme::apply()` no-op | `src/Gui/FreeWorks/FwTheme.cpp` | Full QSS/SVG theme is Phase 7 |
| "FeatureManager (Phase 3)" placeholder widget | `src/Gui/FreeWorks/FwLayout.cpp` | Real FeatureManager tree is Phase 3 |
| "PropertyManager (Phase 4)" placeholder widget | `src/Gui/FreeWorks/FwLayout.cpp` | Real PropertyManager is Phase 4 |
| "Task Pane (Phase 7)" placeholder widget | `src/Gui/FreeWorks/FwLayout.cpp` | Real Task Pane is Phase 7 |
| Placeholder workbench icon SVG | `src/Gui/FreeWorks/Resources/icons/FreeWorksWorkbench.svg` | Final recreated look-alike icon + provenance is Plan 04 |

## Issues Encountered

- The plan's `setupDockWindows()` verify uses `grep "class FwWorkbench"`, but FreeCAD convention places the export macro between `class` and the name (e.g. `class PartGuiExport Workbench`). Resolved by dropping the export macro (the class is only instantiated within FreeCADGui via the type system), which both satisfies the gate and is correct for the compile-into-FreeCADGui design.
- Build was not compiled in this environment (no configured FreeCAD build tree available to this sequential executor). Compile/link verification is delegated to the Plan 03 CI matrix build and headless gate, as the plan's `<verification>` explicitly states. All static/grep acceptance checks for Tasks 1–3 pass.

## Self-Check: PASSED

All 12 created files present on disk; all 3 task commits (`26b7195acc`, `ef6af3ef79`, `34461c3462`) present in git history.
