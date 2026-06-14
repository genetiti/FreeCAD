# Phase 3: FeatureManager Design Tree - Pattern Map

**Mapped:** 2026-06-14
**Files analyzed:** 8 (4 new source, 2 modified source/CMake, 2 new test) + 1 reused test header
**Analogs found:** 8 / 8 (all have a strong in-fork or in-tree analog)

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|-------------------|------|-----------|----------------|---------------|
| `src/Gui/FreeWorks/FwFeatureTree.{h,cpp}` (NEW) | component (widget) | event-driven (observer) | `src/Gui/FreeWorks/FwRibbon.{h,cpp}` (new-file conventions) + `src/Gui/Tree.h` `TreeWidget` (base being subclassed) | exact (dual analog) |
| `src/Gui/FreeWorks/FwFeatureTreeDelegate.{h,cpp}` (NEW, optional) | component (delegate) | transform (display-only remap) | `FwRibbon.{h,cpp}` (conventions); `Gui::TreeWidgetItemDelegate` (role) | role-match |
| `src/Gui/FreeWorks/FwRollbackBar` mechanism (NEW; drawRow override or overlay) | component | event-driven → request-response (command fire) | `CmdPartDesignMoveTip` (`CommandBody.cpp:657-745`) for the fire path; `FwRibbonContext` for the Gui-only-decide-then-act split | role-match |
| `src/Gui/FreeWorks/FwLayout.cpp` (MODIFIED) | config (mount/registration) | request-response | `FwLayout::mountRibbon()` / `ensureDock()` (same file, Phase-2 ribbon mount) | exact |
| `src/Gui/FreeWorks/CMakeLists.txt` (MODIFIED) | config (build) | batch | same file, Phase-2 `FreeWorks_CPP_SRCS`/`HPP_SRCS` append | exact |
| `tests/src/Gui/FwFeatureTree.cpp` (NEW) | test (GTest logic) | request-response | `tests/src/Gui/FwRibbon.cpp` | exact |
| `tests/src/Gui/FwFeatureTreeWidget.cpp` (NEW) | test (QTEST offscreen) | event-driven | `tests/src/Gui/FwRibbonWidget.cpp` | exact |
| `tests/src/Gui/CMakeLists.txt` (MODIFIED) | config (test build) | batch | same file, `Gui_tests_run` list + `setup_qt_test(FwRibbonWidget)` | exact |

---

## Pattern Assignments

### `src/Gui/FreeWorks/FwFeatureTree.{h,cpp}` (component, event-driven)

**Primary analog (new-file conventions):** `src/Gui/FreeWorks/FwRibbon.h`
**Base-class analog (subclass target):** `src/Gui/Tree.h` `TreeWidget`

**License header + pragma + namespace** — copy verbatim from `FwRibbon.h:1-42` and close with `FwRibbon.h:158`:
```cpp
// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2026 FreeWorks contributors                            *
 *   ... (full LGPL-2.1 block, identical to FwRibbon.h:3-23) ...           *
 ***************************************************************************/
#pragma once
#include "PreCompiled.h"
// ... includes ...
namespace FreeWorksGui
{
// ... class ...
}  // namespace FreeWorksGui
```
The header doc-comment convention (purpose + "Compiled directly into FreeCADGui ... so no cross-library export macro is required (mirrors FwWorkbench)") is at `FwRibbon.h:44-56` — reuse that framing.

**Class declaration** — `FwFeatureTree` subclasses the reused engine. The base ctor is public and takes `(const char* name, QWidget* parent = nullptr)` (`Tree.h:65`):
```cpp
class FwFeatureTree: public Gui::TreeWidget
{
    Q_OBJECT
public:
    explicit FwFeatureTree(const char* name, QWidget* parent = nullptr);
    // scoping + presentation hooks here
};
```

**Reusable virtual seams on the base** (from `Tree.h`, the ONLY overridable surface — Pitfall 2):
- `void drawRow(QPainter*, const QStyleOptionViewItem&, const QModelIndex&) const override;` (`Tree.h:147`) → rollback-bar drawn separator + below-tip greying paint.
- DnD overrides already present & reused as-is: `startDrag` (`Tree.h:150`), `dropMimeData` (`Tree.h:151`), `dragEnterEvent`/`dragLeaveEvent`/`dragMoveEvent`/`dropEvent` (`Tree.h:158-161`), `supportedDropActions` (`Tree.h:157`).
- `bool eventFilter(QObject*, QEvent*) override;` (`Tree.h:82`) and the inherited F2 path — do NOT re-implement (D-13).
- `setItemDelegate(...)` (inherited from QTreeWidget) → install `FwFeatureTreeDelegate`.
- **Do NOT** attempt to override `DocumentItem::createNewItem` / `populateItem` — non-virtual, `TreeWidget`-friended (`Tree.h:328-331,456-482`). Finding yourself wanting to is the D-03 FAIL signal.

**Active-Body scoping (App/Gui separation — Pitfall 1):** observe via Gui signals only, identify a Body by the type-name STRING literal — mirror the Phase-2 `FwRibbonContext` `kSketchVpTypeName` precedent (`tests/src/Gui/FwRibbon.cpp:163`):
```cpp
// FreeWorks must NOT include or link PartDesign. Identify the body by string:
constexpr const char* kBodyTypeName = "PartDesign::Body";   // obj->getTypeId().getName()
// observe: Gui::Application::signalActivatedObject / signalInEdit / signalActiveDocument
```

---

### `src/Gui/FreeWorks/FwFeatureTreeDelegate.{h,cpp}` (component, transform)

**Analog:** `FwRibbon.{h,cpp}` for conventions; `Gui::TreeWidgetItemDelegate` (already in use, `Tree.h:50`) for role.
Same license/namespace block as above. A `QStyledItemDelegate` that remaps **display text only** (object `Label` unchanged) for the three origin planes, keyed on the internal `PlaneRoles` name (`Datums.h:204`):

| SW display name | FreeCAD role | FreeCAD label |
|-----------------|--------------|---------------|
| Front Plane | `XY_Plane` | "XY-plane" |
| Top Plane | `XZ_Plane` | "XZ-plane" |
| Right Plane | `YZ_Plane` | "YZ-plane" |

Greying of below-tip rows is Gui-only via `Qt::ForegroundRole` / `QPalette::Disabled` / item flags — NEVER `Visibility.setValue` (Pitfall 3, D-06). No inline hex / `setStyleSheet` (Phase-7 owns color; the Phase-2 palette-only rule, see `02-02-SUMMARY.md` deviation #2 where a `setStyleSheet` comment alone tripped the acceptance grep).

---

### Rollback-bar fire path (component → request-response)

**Analog:** `CmdPartDesignMoveTip::activated` (`src/Mod/PartDesign/Gui/CommandBody.cpp:671-745`).

**Core set-tip sequence to reproduce** (`CommandBody.cpp:730-744`):
```cpp
openCommand(QT_TRANSLATE_NOOP("Command", "Move tip to selected feature"));
if (selFeature == body) {
    FCMD_OBJ_CMD(body, "Tip = None");          // roll to base
} else {
    FCMD_OBJ_CMD(body, "Tip = " << getObjectCmd(selFeature));
    FCMD_OBJ_SHOW(selFeature);
}
updateActive();   // recompute; Body::mustExecute() returns 1 on Tip.isTouched()
```
The only-solid-feature guard (`CommandBody.cpp:712-721`, "Only a solid feature can be the tip") means the bar must snap to the nearest preceding solid feature (`Body::getPrevSolidFeature`) — Pitfall 5.

**App/Gui-separation note (A3):** FreeWorks must NOT C++-call `Body::Tip.setValue` (would link PartDesign). Fire through the existing command-ID `"PartDesign_MoveTip"` (`CommandBody.cpp:660`) via `CommandManager::getCommandByName` with the target feature selected, OR a Python `obj.Tip = ...` `doCommand` string — the exact "reference by string, never link" rule Phase 2 used (`02-01-SUMMARY.md` key-decision, `FwRibbonContext` type-name literal).

**Decide-then-act split (Gui-only state):** mirror `FwRibbonContext`'s pure-logic core (`decideOnEnter`/`decideOnReset` tested headlessly in `tests/src/Gui/FwRibbon.cpp:168-260`) — keep the bar-position→target-feature resolution as pure logic so it is GTest-able without a widget.

**Insert-at-bar:** reuse `Body::insertObject(feature, target, after)` (`Body.h:84`) — "doesn't modify the Tip unlike addObject()". Reorder reuses the Group `PropertyLinkList` path in `TreeWidget::sortDroppedObjects` (`Tree.cpp:3260-3273`) inside one `openTransaction`/`commitTransaction` (D-12).

---

### `src/Gui/FreeWorks/FwLayout.cpp` (config, MODIFIED)

**Analog:** same file — the Phase-2 ribbon mount + the placeholder helpers.

Today the `Fw_FeatureManager` dock holds a placeholder (`FwLayout.cpp:124-127`):
```cpp
ensureDock(manager,
           "Fw_FeatureManager",
           QObject::tr("FeatureManager"),
           QObject::tr("FeatureManager (Phase 3)"));
```
Phase 3 replaces the placeholder content with an `FwFeatureTree` instance. The registration seam is `manager->registerDockWindow(name, widget)` (`FwLayout.cpp:101`). Follow the `mountRibbon()` discipline for the swap:
- **unique_ptr-until-adopt** ownership (a freshly built widget has no QObject parent until `registerDockWindow`/`addWidget` adopts it — `FwLayout.cpp:163-164,191`, "WR-02").
- **idempotent re-activation** — find-or-reuse by objectName, never add a second instance (`FwLayout.cpp:151-153,182-198`).
- Observe-the-DOM only: reach the window via `Gui::getMainWindow()` / `Gui::DockWindowManager::instance()`, never edit `MainWindow.cpp` (`FwLayout.cpp:109-114,140-147`).
- objectName MUST be unique & stable for `saveState()`/`restoreState()` round-trip (`FwLayout.cpp:81-89,60-66`).

---

### `src/Gui/FreeWorks/CMakeLists.txt` (config, MODIFIED)

**Analog:** same file, Phase-2 append. Add the new sources to the existing lists (additive `target_sources(FreeCADGui PRIVATE ...)`, `CMakeLists.txt:17-26,28-37,48`):
```cmake
set(FreeWorks_CPP_SRCS
    ...
    FwRibbonContext.cpp
    FwFeatureTree.cpp          # ADD
    FwFeatureTreeDelegate.cpp  # ADD (if delegate split out)
)
set(FreeWorks_HPP_SRCS
    ...
    FwRibbonContext.h
    FwFeatureTree.h            # ADD
    FwFeatureTreeDelegate.h    # ADD
)
```
No `target_link_libraries` to PartDesign/Sketcher (the no-module-link rule).

---

### `tests/src/Gui/FwFeatureTree.cpp` (test, GTest logic)

**Analog:** `tests/src/Gui/FwRibbon.cpp`.

**SetUpTestSuite bootstrap call** — reuse verbatim (`FwRibbon.cpp:43-58`):
```cpp
#include "FwTestGuiBootstrap.h"
class FwFeatureTreeTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::ensureGuiTestBootstrap();   // App+QApp(offscreen)+Gui::Application+module imports
    }
};
```
`ensureGuiTestBootstrap()` already creates `Gui::Application(false)` then imports `PartDesignGui`/`SketcherGui`/`MeasureGui`/`MatGui` (`FwTestGuiBootstrap.h:54-105`) — so `PartDesign::Body`, the feature types, and `PartDesign_MoveTip` all resolve headlessly. Reuse the header as-is (no edit).

**Tests to author (pure-logic, per RESEARCH Wave-0 + Validation map):** type-name scoping resolves `"PartDesign::Body"`; set-tip-to-earlier → `Tip.isTouched()` → `mustExecute()==1`; `insertObject` placement in Group; reorder via Group `PropertyLinkList` + undo restores; DnD gate `canDragObjectToTarget`/`canDropObjectEx` returns false for child-before-parent (BLOCK). Follow the `FwRibbon.cpp` pattern of an intentionally-empty allow-list guard and per-row asserts where applicable.

---

### `tests/src/Gui/FwFeatureTreeWidget.cpp` (test, QTEST offscreen)

**Analog:** `tests/src/Gui/FwRibbonWidget.cpp`.

QTEST_MAIN class wired identically (`FwRibbonWidget.cpp:50-69`):
```cpp
#include <QTest>
#include "FwTestGuiBootstrap.h"
class testFwFeatureTreeWidget: public QObject
{
    Q_OBJECT
public:
    testFwFeatureTreeWidget() { tests::ensureGuiTestBootstrap(); }
private Q_SLOTS:
    void init() {}
    void cleanup() {}
    // tree construction; Origin-first/creation-order/nested structure;
    // delegate plane-label remap; F2 action present
};
```
If the widget test needs a real registered window, reuse the `ensureRealMainWindow()` helper pattern (`FwRibbonWidget.cpp:37-43`) — `new Gui::MainWindow()` sets the singleton; deliberately leaked to outlive the QTEST process.

---

### `tests/src/Gui/CMakeLists.txt` (config, MODIFIED)

**Analog:** same file (`CMakeLists.txt:7-23`). Add the GTest source to the `Gui_tests_run` list and register the QTest target via `setup_qt_test`:
```cmake
add_executable(Gui_tests_run
        ...
        FwRibbon.cpp
        FwFeatureTree.cpp        # ADD
)
setup_qt_test(QuantitySpinBox)
setup_qt_test(FwRibbonWidget)
setup_qt_test(FwFeatureTreeWidget)   # ADD
```

---

## Shared Patterns

### New-file boilerplate (license + namespace)
**Source:** `src/Gui/FreeWorks/FwRibbon.h:1-42,158`
**Apply to:** every new `Fw*.{h,cpp}` source file. Identical SPDX line, LGPL-2.1 block, `#pragma once`, `#include "PreCompiled.h"`, `namespace FreeWorksGui { ... }`.

### Reference-by-string, never-link (App/Gui separation + merge isolation)
**Source:** `02-01-SUMMARY.md` key-decision; `FwRibbonContext` type-name literal (`tests/src/Gui/FwRibbon.cpp:163`)
**Apply to:** FwFeatureTree scoping (`"PartDesign::Body"`), rollback fire (`"PartDesign_MoveTip"` command-ID or Python `doCommand`). NO `#include <Mod/PartDesign/...>`, NO `target_link_libraries(... PartDesignGui)`. Enforced by `tools/fw-string-leak-grep.sh` (allow-lists only `Gui::SolidWorksNavigationStyle`); the "SolidWorks" identifier is forbidden in source — SW plane names live in `tr()` display text only.

### Transactioned mutation (clean undo, no .FCStd pollution)
**Source:** `CommandBody.cpp:730-744` (`openCommand`/`updateActive`); `Tree.cpp:3260-3273` reorder
**Apply to:** every App-state change — set Tip, reorder, rename, insert. `Document::openTransaction(...)` → Property setter → `commitTransaction()`. Never mutate during recompute; never write `Visibility` for greying.

### Headless test bootstrap (reuse verbatim)
**Source:** `tests/src/Gui/FwTestGuiBootstrap.h` (no edit)
**Apply to:** both new test files via `tests::ensureGuiTestBootstrap()` in `SetUpTestSuite()` (GTest) or the ctor (QTEST). It already loads PartDesign/Sketcher/Measure/Mat GUI modules so Body/feature types and `PartDesign_MoveTip` resolve.

### Verification-deferral (no build tree here)
**Source:** `02-01-SUMMARY.md` / `SPIKE_LIVE_CHECKLIST.md` pattern
**Apply to:** authored tests are CI-green-intended; live items (3D suppress-below feel, insertion-line cursor, SC5 parity) go to a `SPIKE_LIVE_CHECKLIST` — never fabricated. Record the D-03 verdict (`reuse committed` vs `subclass-fallback committed`) in a `SPIKE.md`, mirroring Phase-2's `native committed`.

## No Analog Found

| File | Role | Data Flow | Reason |
|------|------|-----------|--------|
| (none) | — | — | Every new/modified file has a strong in-fork (Phase-2 FreeWorks) or in-tree (Tree/Body/CommandBody) analog. The D-03 spike resolves only the *degree* of subclassing, not a missing analog. |

## Metadata

**Analog search scope:** `src/Gui/FreeWorks/`, `src/Gui/Tree.{h,cpp}`, `src/Mod/PartDesign/{App,Gui}/`, `tests/src/Gui/`
**Files read:** FwRibbon.h, FwLayout.cpp, FreeWorks/CMakeLists.txt, tests/FwRibbon.cpp, tests/FwRibbonWidget.cpp, FwTestGuiBootstrap.h, tests/CMakeLists.txt, Tree.h (TreeWidget decl), CommandBody.cpp (CmdPartDesignMoveTip), Body.h (insertObject) + the two upstream planning docs
**Pattern extraction date:** 2026-06-14
