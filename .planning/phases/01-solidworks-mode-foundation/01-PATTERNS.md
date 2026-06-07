# Phase 1: SolidWorks Mode Foundation - Pattern Map

**Mapped:** 2026-06-06
**Files analyzed:** 13 (new) + 2 (shared-file edits)
**Analogs found:** 11 / 13 (2 greenfield repo-tooling files have no direct analog)

> Naming authority: **D-02 `Fw` / `FreeWorksGui` / `src/Gui/FreeWorks/`** overrides every `Sw*` example in RESEARCH.md. The ONLY legitimate "SolidWorks" identifier in new code is the existing upstream type-name string `"Gui::SolidWorksNavigationStyle"` (D-03 exception).

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|-------------------|------|-----------|----------------|---------------|
| `src/Gui/FreeWorks/FwWorkbench.{h,cpp}` | workbench | event-driven (activate) | `src/Mod/Part/Gui/Workbench.{h,cpp}` + `Gui::StdWorkbench` (`src/Gui/Workbench.cpp:918`) | exact |
| `src/Gui/FreeWorks/FwLayout.{h,cpp}` | provider/utility | request-response (install into MainWindow) | `Gui::BlankWorkbench::activated()` (`src/Gui/Workbench.cpp:955`) + `DockWindowManager` | role-match |
| `src/Gui/FreeWorks/FwTheme.{h,cpp}` | utility/config | transform (QSS apply) | `StyleParameters` (`tests/src/Gui/StyleParameters/`) | partial |
| `src/Gui/FreeWorks/FwNavigationDefault.{h,cpp}` | config/utility | request-response (param read/write) | `View3DSettings.cpp:279-288` param-read pattern | role-match |
| `src/Gui/FreeWorks/PreCompiled.{h,cpp}` | config | n/a | any module `PreCompiled.{h,cpp}` (e.g. Part Gui) | exact |
| `src/Gui/FreeWorks/Resources/*.qrc` + icon | resource/config | n/a | `qt_add_resources` in `src/Gui/CMakeLists.txt:447` | exact |
| `src/Gui/FreeWorks/CMakeLists.txt` | config | n/a | `src/Gui/CMakeLists.txt` SET-source + add_subdirectory blocks | role-match |
| `src/Gui/CMakeLists.txt` (EDIT) | config | n/a | existing `add_subdirectory(...)` (`:14-16`) | exact |
| Workbench registration (`InitGui.py` or C++ `addWorkbench`) | config | event-driven | `src/Mod/Part/InitGui.py:32-75` (`Gui.addWorkbench`) | exact |
| `tests/src/Gui/FwWorkbench.cpp` (GTest) | test | n/a | `tests/src/Gui/SelectionTest.cpp` | exact |
| `tests/src/Gui/CMakeLists.txt` (EDIT) | config | n/a | `tests/src/Gui/CMakeLists.txt:7-16` source list | exact |
| `.github/workflows/` provenance + headless gates | CI/config | event-driven (CI) | `.github/workflows/sub_lint.yml` (`workflow_call` job pattern) | role-match |
| `ASSET_PROVENANCE.md` | doc | n/a | — (greenfield) | none |
| `tools/fw-sync-upstream.sh` | tooling | batch | — (greenfield) | none |

## Pattern Assignments

### `src/Gui/FreeWorks/FwWorkbench.{h,cpp}` (workbench, event-driven)

**Primary analog:** `src/Mod/Part/Gui/Workbench.h` (header shape) + `Gui::StdWorkbench` in `src/Gui/Workbench.cpp` (dock/typesystem body).

**Header pattern** — copy structure from `src/Mod/Part/Gui/Workbench.h:1-57`:
- SPDX line 1: `// SPDX-License-Identifier: LGPL-2.1-or-later` (mandatory, every new file).
- `#pragma once`, `#include <Gui/Workbench.h>`, module-export macro, `namespace FreeWorksGui {`.
- `class FwWorkbench: public Gui::StdWorkbench { TYPESYSTEM_HEADER_WITH_OVERRIDE(); ... };`
- Subclass **`StdWorkbench`** (not bare `Workbench`) to inherit the Std command set — confirmed by Part doing exactly this and by RESEARCH Pattern 1.

**Override signatures** (from `src/Gui/Workbench.h:116-123, 164-205`):
```cpp
Gui::MenuItem* setupMenuBar() const override;
Gui::ToolBarItem* setupToolBars() const override;
Gui::ToolBarItem* setupCommandBars() const override;
DockWindowItems* setupDockWindows() const override;   // Phase 1 core deliverable
void activated() override;                              // installs FwLayout
```

**TYPESYSTEM_SOURCE pattern** (cpp) — from `src/Gui/Workbench.cpp:623`:
```cpp
TYPESYSTEM_SOURCE(FreeWorksGui::FwWorkbench, Gui::StdWorkbench)
```

**setupDockWindows() pattern** — adapt `Gui::StdWorkbench::setupDockWindows()` (`src/Gui/Workbench.cpp:918-943`):
```cpp
DockWindowItems* FwWorkbench::setupDockWindows() const {
    auto root = new DockWindowItems();
    // Register PERMANENT Fw dock NAMES (OQ-2) — later phases swap content, not names.
    root->addDockWidget("Fw_FeatureManager",  Qt::LeftDockWidgetArea,  Gui::DockWindowOption::Visible);
    root->addDockWidget("Fw_PropertyManager", Qt::LeftDockWidgetArea,  Gui::DockWindowOption::Visible);
    // reserve Fw_TaskPane on the right; TOP area left free for the P2 ribbon
    root->addDockWidget("Fw_TaskPane",        Qt::RightDockWidgetArea, Gui::DockWindowOption::VisibleTabbed);
    return root;
}
```
Note: the literal strings `"Std_TreeView"` etc. in the analog are the upstream dock names — DO NOT edit those Std blocks (Pattern 4 / Pitfall 1). Use new `Fw_*` names only.

**activated() pattern** — model on `Gui::BlankWorkbench::activated()` (`src/Gui/Workbench.cpp:955-962`, which iterates `getMainWindow()->findChildren<QDockWidget*>()`):
```cpp
void FwWorkbench::activated() {
    FwLayout::install(Gui::getMainWindow());   // apply coherent SW dock geometry
    StdWorkbench::activated();
}
```

---

### `src/Gui/FreeWorks/FwLayout.{h,cpp}` (provider/utility, request-response)

**Analog:** `Gui::BlankWorkbench::activated()` (`src/Gui/Workbench.cpp:955-962`) for the `getMainWindow()` + `findChildren<QDockWidget*>()` access pattern; `DockWindowManager` (`src/Gui/DockWindowManager.h:84`) for placeholder widget registration.

**Dock-widget registration pattern** (`src/Gui/DockWindowManager.h:84`):
```cpp
bool registerDockWindow(const char* name, QWidget* widget);
```
Use this to back each `Fw_*` dock name with a labeled placeholder `QWidget` (OQ-2). Register under the permanent `Fw_*` name so later phases swap content without re-layout.

**Constraint:** FwLayout reads `Gui::getMainWindow()` and arranges docks; it must NOT edit `MainWindow.cpp`. All MainWindow interaction is via the public `getMainWindow()` getter + Qt dock APIs (observe-the-DOM, RESEARCH Pattern 2).

---

### `src/Gui/FreeWorks/FwNavigationDefault.{h,cpp}` (config, request-response)

**Analog:** the parameter-group read in `src/Gui/View3DSettings.cpp:279-288` (key `"NavigationStyle"`, group `User parameter:BaseApp/Preferences/View`, current default `CADNavigationStyle`).

**Core pattern** (verified in RESEARCH §Code Examples):
```cpp
auto hGrp = App::GetApplication()
    .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
hGrp->SetASCII("NavigationStyle", "Gui::SolidWorksNavigationStyle");   // SW-FORK HOOK
```
- This is the ONE place the literal `"SolidWorks"` string is permitted (D-03 exception — upstream type name).
- Set as a default only when the key is unset (don't clobber a user's chosen nav style — Runtime State Inventory note A2).
- Mark with `// SW-FORK HOOK` so the sync drill greps it.
- macOS substitute (OQ-3): also expose `"Gui::GestureNavigationStyle"` / modifier-emulated MMB — Gui-only.

---

### `src/Gui/FreeWorks/PreCompiled.{h,cpp}` (config)

**Analog:** any FreeCAD module's `PreCompiled.{h,cpp}` (e.g. `src/Mod/Part/Gui/PreCompiled.h`). Copy the include-guard + common-header block verbatim, adapting the export-macro name to the FreeWorks module. SPDX header on line 1.

---

### `src/Gui/FreeWorks/CMakeLists.txt` + `Resources/*.qrc` (config/resource)

**Analog:** `src/Gui/CMakeLists.txt` — `qt_add_resources(Gui_QRC_SRCS ${Gui_RES_SRCS})` (`:447`) for the icon `.qrc`, and the `SET(..._SRCS ...)` + `add_library`/`target_link_libraries` pattern used throughout. The workbench icon is **recreated original art** (Legal constraint; must get an `ASSET_PROVENANCE.md` row before it lands).

---

### `src/Gui/CMakeLists.txt` (EDIT — the primary shared-file touch)

**Analog:** existing `add_subdirectory(...)` lines at `src/Gui/CMakeLists.txt:14-16`.

**Edit:** add one line, marked:
```cmake
add_subdirectory(FreeWorks)  # SW-FORK HOOK
```
This is one of the at-most-two unavoidable shared-file edits (RESEARCH Summary). Keep it to a single greppable line.

---

### Workbench registration (`InitGui.py` Python path OR C++ `addWorkbench`)

**Analog:** `src/Mod/Part/InitGui.py:32-75`:
```python
class FreeWorksWorkbench(Gui.Workbench):
    def __init__(self):
        self.__class__.Icon = os.path.join(App.getResourceDir(), "Gui", "FreeWorks", ...)
        self.__class__.MenuText = "FreeWorks"      # D-02: label is "FreeWorks"
        self.__class__.ToolTip  = "FreeWorks mode"
    def GetClassName(self):
        return "FreeWorksGui::FwWorkbench"          # maps to the C++ class
Gui.addWorkbench(FreeWorksWorkbench())              # InitGui.py:75
```
- `MenuText`/`ToolTip`/label = "FreeWorks" (NO "SolidWorks" — D-03).
- `GetClassName()` returns the qualified C++ workbench class name `FreeWorksGui::FwWorkbench`.

---

### `tests/src/Gui/FwWorkbench.cpp` (GTest — covers SC1, SC3)

**Analog:** `tests/src/Gui/SelectionTest.cpp:1-55`.

**Fixture pattern** to copy:
- SPDX line 1; `#include <gtest/gtest.h>`; `#include <src/App/InitApplication.h>`.
- `class FwWorkbenchTest : public ::testing::Test` with `static void SetUpTestSuite() { tests::initApplication(); }` (`SelectionTest.cpp:42-45`).
- Keep headless: use `App::DocumentInitFlags{ .createView = false }` style as in `SelectionTest.cpp:51-55`.

**Assertions (Wave 0 gaps from RESEARCH §Validation):**
- `FwWorkbench` registers and `setupDockWindows()` returns docks named `Fw_*` (SC1).
- `NavigationStyle` default resolves to `Gui::SolidWorksNavigationStyle` (SC3).

**Register in test build:** add `FwWorkbench.cpp` to the `add_executable(Gui_tests_run ...)` source list (`tests/src/Gui/CMakeLists.txt:7-16`). Target already links `FreeCADGui` (`:23`).

---

### `.github/workflows/` provenance + headless gates (CI)

**Analog:** `.github/workflows/sub_lint.yml:30-40` — `workflow_call` reusable-job structure (`on: workflow_call:` with typed `inputs:`). New jobs follow the same SPDX-headered, `workflow_call` shape and slot into `CI_primary.yml` like `sub_lint`.

**Jobs to add (Wave 0):**
- Asset-provenance guard: diff added binary images vs `ASSET_PROVENANCE.md` → fail if any unlisted (SC4a).
- Headless `.FCStd` gate: save in FreeWorks mode → `FreeCADCmd -c "...openDocument...recompute..."` → assert `HEADLESS_OK` and no Fw/Gui keys; opens in unmodified upstream (SC4b). `MainCmd.cpp` confirmed present.
- "SolidWorks"-leak grep: fail on `SolidWorks` in new `src/Gui/FreeWorks/` identifiers/strings, allow-listing the single `"Gui::SolidWorksNavigationStyle"` reference (Pitfall 4).
- Unmarked-shared-edit grep: assert no `MainWindow.cpp`/shared diffs outside `// SW-FORK HOOK` (SC1/SC5).

---

## Shared Patterns

### SPDX license header (ALL new files)
**Source:** `src/Mod/Part/Gui/Workbench.h:1`, `tests/src/Gui/SelectionTest.cpp:1`
**Apply to:** every new `.h/.cpp/.txt/.yml/.sh/.md`
```cpp
// SPDX-License-Identifier: LGPL-2.1-or-later
```
(CMake/YAML/shell use `# SPDX-License-Identifier: LGPL-2.1-or-later`.)

### TYPESYSTEM registration (C++ workbench)
**Source:** `src/Gui/Workbench.cpp:623` + `src/Gui/Workbench.h:153`
**Apply to:** `FwWorkbench`
- Header: `TYPESYSTEM_HEADER_WITH_OVERRIDE();`
- Cpp: `TYPESYSTEM_SOURCE(FreeWorksGui::FwWorkbench, Gui::StdWorkbench)`

### `// SW-FORK HOOK` merge-discipline marker
**Source:** none exists yet (greenfield — RESEARCH verified zero markers in tree)
**Apply to:** every unavoidable shared-file edit — exactly two expected: `src/Gui/CMakeLists.txt` add_subdirectory, and the `NavigationStyle` default in `FwNavigationDefault.cpp`. The sync script and CI grep enumerate these.

### Observe-the-DOM, never mutate App directly
**Source:** project ARCHITECTURE.md anti-patterns + RESEARCH Pattern 2
**Apply to:** `FwLayout`, `FwWorkbench`, all FreeWorks widgets — read via `getMainWindow()`/signals/getters; write only via `Gui::Command` / property setters / `Control().accept()`. No App-layer `#include`, no new App properties (Pitfall 2; SC4b headless gate is the detector).

### Naming / namespace (D-02)
**Apply to:** all new code — `Fw`-prefixed `PascalCase` classes, `FreeWorksGui` namespace, `src/Gui/FreeWorks/` path, private members `_lowerCamelCase`, methods `camelCase`, 4-space / 100-col clang-format.

## No Analog Found

| File | Role | Data Flow | Reason |
|------|------|-----------|--------|
| `ASSET_PROVENANCE.md` | doc/ledger | n/a | No provenance ledger exists in repo yet (greenfield, RESEARCH-verified). Author from RESEARCH §Pitfall 5 spec: one row per binary image → original source. |
| `tools/fw-sync-upstream.sh` | git tooling | batch | No `tools/` sync script + no pinned-commit drill exists yet (greenfield). Author from RESEARCH SC5: pinned-commit sync, mirror branch, `grep -rn "SW-FORK HOOK" src/`. |

## Metadata

**Analog search scope:** `src/Gui/` (Workbench, DockWindowManager, View3DSettings, Navigation), `src/Mod/Part/{Gui,InitGui.py}`, `tests/src/Gui/`, `.github/workflows/`, `src/Main/`.
**Files scanned:** ~12 source/config files read; greps across `src/Gui`, `tests`, `.github`.
**Pattern extraction date:** 2026-06-06
