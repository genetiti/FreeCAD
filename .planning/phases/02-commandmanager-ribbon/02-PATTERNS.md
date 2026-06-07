# Phase 2: CommandManager Ribbon - Pattern Map

**Mapped:** 2026-06-07
**Files analyzed:** 8 (5 new C++ pairs/files, 2 extended, 1 new test, plus CMake edits)
**Analogs found:** 8 / 8 (all have strong in-tree analogs)

> All new code lives under `src/Gui/FreeWorks/`, namespace `FreeWorksGui`, `Fw` class prefix.
> Compiled directly into `FreeCADGui` (no new lib). Observe-the-DOM: read registry/edit-state
> via getters/signals, never mutate App. No "SolidWorks" identifier strings (CI leak-grep).
> Any unavoidable shared-file edit gets `// SW-FORK HOOK`.

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|-------------------|------|-----------|----------------|---------------|
| `src/Gui/FreeWorks/FwRibbon.{h,cpp}` | widget/provider | transform (registry tree → widgets) | `src/Gui/FreeWorks/FwLayout.cpp` (build-from-DOM) + `src/Gui/Action.cpp` (addTo) | role-match |
| `src/Gui/FreeWorks/FwRibbonMap.{h,cpp}` | config/data | transform (static table → cmd IDs) | `src/Mod/PartDesign/Gui/Workbench.cpp::setupToolBars()` | role-match (data source) |
| `src/Gui/FreeWorks/FwRibbonContext.{h,cpp}` | provider/observer | event-driven | `src/Gui/OverlayManager.cpp:406-409` (signalInEdit/ResetEdit subscriber) | exact |
| `src/Gui/FreeWorks/FwLayout.{h,cpp}` (EXTEND) | service/installer | request-response (install) | existing `FwLayout.cpp` (Phase 1) | exact (self) |
| `src/Gui/FreeWorks/FwWorkbench.{h,cpp}` (EXTEND) | workbench/lifecycle | event-driven (activated/deactivated) | existing `FwWorkbench.cpp` (Phase 1) | exact (self) |
| `tests/src/Gui/FwRibbon.cpp` | test | unit | `tests/src/Gui/FwWorkbench.cpp` (Phase 1 GTest) | exact |
| `src/Gui/FreeWorks/CMakeLists.txt` (EXTEND) | config | — | existing `CMakeLists.txt` (`target_sources`) | exact (self) |
| `tests/src/Gui/CMakeLists.txt` (EXTEND) | config | — | existing (add source to `Gui_tests_run`) | exact (self) |

---

## Pattern Assignments

### `src/Gui/FreeWorks/FwRibbon.{h,cpp}` (widget, transform)

The `QTabWidget` shell. Builds tab pages of `QToolBar` panels from command IDs, mounts in
`TopToolBarArea`, exposes `setCurrentTab(name)` / `currentIndex()` for the context switcher.

**Header license + PreCompiled + namespace** — copy the exact header block and structure from
`src/Gui/FreeWorks/FwLayout.h:1-48` (SPDX line 1, the 23-line copyright block, `#pragma once`,
`#include "PreCompiled.h"`, `namespace FreeWorksGui { ... }`). For an exported class use the
`FreeWorksGuiExport` macro as `FwLayout` does (`FwLayout.h:41`); a type-system class compiled
only into FreeCADGui needs no export (see `FwWorkbench.h:47-51`).

**Large labeled ribbon button from a registry command** — RESEARCH Pattern 1, verified APIs:
- `Gui::Command* cmd = Gui::Application::Instance->commandManager().getCommandByName(id);`
  (`src/Gui/Command.h:1011`)
- `cmd->addTo(panel);` adds the command's existing `QAction` (`src/Gui/Command.h:421`)
- panel = `QToolBar` with `setToolButtonStyle(Qt::ToolButtonTextUnderIcon)` and
  `setIconSize(QSize(32,32))` (UI-SPEC large-icon metric; icon-over-label = D-13)
- D-08 omit-missing: skip the id when `getCommandByName` returns `nullptr`.

**Flyout split-button — reuse, do NOT hand-roll** — putting a `*_Comp*` group command ID (e.g.
`PartDesign_CompPrimitiveAdditive`, `Sketcher_CompLine`) in the map and calling the same
`cmd->addTo(panel)` produces a native split-button. The mechanics live in
`src/Gui/Action.cpp:472-485` (`ActionGroup::addTo` for a `QToolBar`):
```cpp
widget->addAction(action());
QToolButton* tb = widget->findChildren<QToolButton*>().constLast();
tb->setPopupMode(QToolButton::MenuButtonPopup);
auto menu = new QMenu(tb);
menu->addActions(groupAction()->actions());
tb->setMenu(menu);
```
The ribbon invents nothing here — the curated map's only lever is *which IDs* go in which panel.

**Auto-derive panels for uncurated workbenches (D-07)** — walk the workbench's `ToolBarItem`
tree (`src/Gui/ToolBarManager.h:73-87`): each top-level `getItems()` group → a panel; each
child `leaf->command()` ("" => separator) → a button (RESEARCH "Auto-derive" example).

**Unique objectName for persistence (Pitfall 4 / D-14)** — give the ribbon `Fw_Ribbon` and each
persisted panel `Fw_RibbonPanel_<Tab>`; mirrors the Phase-1 `Fw_*` unique-objectName discipline
documented at `FwLayout.cpp:43-50,90-105`. `QMainWindow::saveState()/restoreState()` key by it.

---

### `src/Gui/FreeWorks/FwRibbonMap.{h,cpp}` (config/data, transform)

Declarative C++ table (`std::array`/`constexpr` rows of `{tab, panel, commandId}`) — RESEARCH
"Curated-Map File Format" recommendation (compile-time, no parse/IO, no `.qrc`).

**Analog for the actual command IDs and grouping** — `src/Mod/PartDesign/Gui/Workbench.cpp`
`setupToolBars()` enumerates the exact registered IDs the curated map reproduces. Verified rows
to draw from (`Workbench.cpp:487-526`):
```cpp
*sketch        << "PartDesign_NewSketch";
*additives     << "PartDesign_Pad" << "PartDesign_Revolution" << "PartDesign_AdditiveLoft"
               << "PartDesign_AdditivePipe" << "PartDesign_AdditiveHelix";
*subtractives  << "PartDesign_Pocket" << "PartDesign_Hole" << "PartDesign_Groove"
               << "PartDesign_SubtractiveLoft" << "PartDesign_SubtractivePipe" << "PartDesign_SubtractiveHelix";
*transformations << "PartDesign_Mirrored" << "PartDesign_LinearPattern" << "PartDesign_PolarPattern";
```
Dress-up IDs at `Workbench.cpp:177,208` (`PartDesign_Fillet`, `PartDesign_Chamfer`, …). Sketcher
geometry/lifecycle IDs from `src/Mod/Sketcher/Gui/Workbench.cpp` (verify constraint IDs in
`Sketcher/Gui/CommandConstraints.cpp` and Measure IDs at plan time — RESEARCH OpenQ 2 / A2).
D-08: place only real IDs; flyouts via the `*_Comp*` IDs.

---

### `src/Gui/FreeWorks/FwRibbonContext.{h,cpp}` (provider/observer, event-driven)

Subscribes to the app-level edit signals; on sketch enter remembers + jumps to Sketch (D-09/D-10),
on reset restores. **NOT polling** `Control::activeDialog()` (Pitfall 1).

**Exact analog** — `src/Gui/OverlayManager.cpp:406-409`:
```cpp
Application::Instance->signalInEdit.connect([this](const ViewProviderDocumentObject&) { refresh(); });
Application::Instance->signalResetEdit.connect([this](const ViewProviderDocumentObject&) { refresh(); });
```
Signal declarations (`src/Gui/Application.h:153-156`):
```cpp
fastsignals::signal<void(const Gui::ViewProviderDocumentObject&)> signalInEdit;
fastsignals::signal<void(const Gui::ViewProviderDocumentObject&)> signalResetEdit;
```
Sketch identity by type-name string — NO Sketcher include (Pitfall 2):
`std::string(vp.getTypeId().getName()) == "SketcherGui::ViewProviderSketch"`. Store the returned
`fastsignals` connections as members so they disconnect on teardown (scoped-connection pattern;
`ToolBarAreaWidget` takes `fastsignals::advanced_scoped_connection&`, `ToolBarAreaWidget.h:55`).

---

### `src/Gui/FreeWorks/FwLayout.{h,cpp}` (service/installer) — EXTEND

Existing `install()` registers placeholder docks (`FwLayout.cpp:77-106`). Extend to (a) build +
mount `FwRibbon` and (b) hide stock chrome reversibly.

**Mount in the reserved top area** — obtain the top `ToolBarAreaWidget` via `ToolBarManager`
(`src/Gui/ToolBarManager.h:181` `toolBarAreaWidget(QWidget*)`, area enum value
`ToolBarArea::TopToolBarArea` at `ToolBarAreaWidget.h:41`), then `topArea->addWidget(fwRibbon)`
(`ToolBarAreaWidget.h:48`). Follow the existing observe-the-DOM note (`FwLayout.cpp:78-87`):
operate only through public singletons, never edit `MainWindow.cpp`.

**Hide stock chrome reversibly (D-11, Pitfall 3)** —
`Gui::ToolBarManager::getInstance()->setState(names, State::ForceHidden)` and
`getMainWindow()->menuBar()->hide()`; restore with `State::RestoreDefault` + `menuBar()->show()`.
State enum: `src/Gui/ToolBarManager.h:154-158` (`ForceHidden`, `RestoreDefault`); setter
`ToolBarManager.h:174-175`.

---

### `src/Gui/FreeWorks/FwWorkbench.{h,cpp}` (workbench/lifecycle) — EXTEND

`activated()` currently does nav/theme then `StdWorkbench::activated()` (`FwWorkbench.cpp:44-57`).
Extend to build the ribbon + hide chrome; add a deactivation path that restores chrome
(Pitfall 3 — hide on activate, restore on deactivate). `setupToolBars()`/`setupMenuBar()` keep
deferring to Std (`FwWorkbench.cpp:59-76`) since the ribbon, not these overrides, is the surface.
Keep the existing `setupDockWindows()` (`FwWorkbench.cpp:78-108`) — its top area stays free for
the ribbon (note at `FwWorkbench.cpp:96`).

---

### `tests/src/Gui/FwRibbon.cpp` (test, unit)

**Exact analog** — `tests/src/Gui/FwWorkbench.cpp` (whole file). Reuse:
- includes + `tests::initApplication()` in `SetUpTestSuite()` (`FwWorkbench.cpp:1-17,47-50`)
- protected-member access via a `using`-re-publishing accessor subclass
  (`FwWorkbench.cpp:38-42`) — the project convention for testing protected members
- `TEST_F` structure with `constexpr const char*` ID constants (`FwWorkbench.cpp:24-26,80`)

Cover (RESEARCH Test Map): every curated ID resolves via `getCommandByName` (or is a recorded
gap); ribbon builds ≥3 tabs headless; a flyout group resolves to an `ActionGroup` with >1 action;
the switch function maps `"SketcherGui::ViewProviderSketch"` → Sketch index and reset → previous;
`QMainWindow` state save→restore preserves `Fw_Ribbon`; workbench toggle hides then restores chrome.

**Register the test** — add `FwRibbon.cpp` to the `Gui_tests_run` `add_executable` list in
`tests/src/Gui/CMakeLists.txt:7-18` (right after the existing `FwWorkbench.cpp` line). Links are
already wired (`FreeCADGui`, GTest, `tests/src/Gui/CMakeLists.txt:21-27`).

---

### `src/Gui/FreeWorks/CMakeLists.txt` — EXTEND

Append the new sources to `FreeWorks_CPP_SRCS` / `FreeWorks_HPP_SRCS` (existing pattern,
`CMakeLists.txt:17-31`); they flow into `target_sources(FreeCADGui PRIVATE ...)` at line 42. No
new link target, no shared-file edit (only `src/Gui/CMakeLists.txt` `add_subdirectory`, already
wired in Phase 1). SARibbon fallback (D-04) would add `add_subdirectory(SARibbon)` marked
`// SW-FORK HOOK` — only on spike failure.

---

## Shared Patterns

### File header / module conventions
**Source:** `src/Gui/FreeWorks/FwLayout.h:1-48`, `FwLayout.cpp:1-36`
**Apply to:** every new `.h`/`.cpp`
SPDX line, the 23-line LGPL copyright block, `#include "PreCompiled.h"` first, explicit Qt
includes (do not rely on PCH — `FwLayout.cpp:27-30`), `using namespace FreeWorksGui;` in `.cpp`,
all symbols inside `namespace FreeWorksGui`.

### Command firing / metadata (observe-the-DOM)
**Source:** `src/Gui/Command.h:421` (`addTo`), `:1011` (`getCommandByName`), `:1016`
(`runCommandByName`), `:1005` (`getGroupCommands`), `:993` (`getModuleCommands`)
**Apply to:** `FwRibbon`, `FwRibbonContext`, discoverability overflow (D-12)
Buttons are thin triggers — reuse existing `QAction`s; never duplicate command backend (SC2).

### Edit-signal subscription (event-driven, scoped)
**Source:** `src/Gui/OverlayManager.cpp:406-409`, signals at `src/Gui/Application.h:153-156`
**Apply to:** `FwRibbonContext`
Subscribe once via `Application::Instance->signal...connect(lambda)`; store + release connections.

### Unique objectName for persistence
**Source:** `src/Gui/FreeWorks/FwLayout.cpp:43-59,90-105` (CR-02 note)
**Apply to:** `FwRibbon` (`Fw_Ribbon`) and each persisted panel (`Fw_RibbonPanel_*`)
Qt keys `saveState()/restoreState()` by objectName; empty/duplicate = silent no-restore.

### Reversible, FreeWorks-scoped chrome hiding
**Source:** `src/Gui/ToolBarManager.h:154-158,174-175` + `MainWindow::menuBar()`
**Apply to:** `FwLayout`/`FwWorkbench`
Hide on activate (`ForceHidden` + `menuBar()->hide()`), restore on deactivate (`RestoreDefault`
+ `menuBar()->show()`) so stock workbenches keep their chrome (Pitfall 3).

### Headless GTest scaffold
**Source:** `tests/src/Gui/FwWorkbench.cpp:38-50`
**Apply to:** `tests/src/Gui/FwRibbon.cpp`
`tests::initApplication()` in `SetUpTestSuite`; protected access via re-publishing subclass.

## No Analog Found

None. Every new file has a strong in-tree analog (Phase-1 FreeWorks files, `Action.cpp` flyout,
`OverlayManager.cpp` signal subscriber, PartDesign/Sketcher `Workbench.cpp` command data,
Phase-1 GTest).

## Metadata

**Analog search scope:** `src/Gui/FreeWorks/`, `src/Gui/{Action,Command,ToolBarManager,
ToolBarAreaWidget,OverlayManager,Application,MainWindow}.{h,cpp}`, `src/Mod/{PartDesign,Sketcher}/
Gui/Workbench.cpp`, `tests/src/Gui/`
**Files scanned:** ~16
**Pattern extraction date:** 2026-06-07
