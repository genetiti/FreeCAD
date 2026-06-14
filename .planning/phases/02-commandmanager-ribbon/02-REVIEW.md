---
phase: 02-commandmanager-ribbon
reviewed: 2026-06-13T00:00:00Z
depth: deep
files_reviewed: 13
files_reviewed_list:
  - src/Gui/FreeWorks/FwRibbon.h
  - src/Gui/FreeWorks/FwRibbon.cpp
  - src/Gui/FreeWorks/FwRibbonMap.h
  - src/Gui/FreeWorks/FwRibbonMap.cpp
  - src/Gui/FreeWorks/FwRibbonContext.h
  - src/Gui/FreeWorks/FwRibbonContext.cpp
  - src/Gui/FreeWorks/FwLayout.h
  - src/Gui/FreeWorks/FwLayout.cpp
  - src/Gui/FreeWorks/FwWorkbench.h
  - src/Gui/FreeWorks/FwWorkbench.cpp
  - tests/src/Gui/FwRibbon.cpp
  - tests/src/Gui/FwRibbonWidget.cpp
  - tests/src/Gui/FwTestGuiBootstrap.h
findings:
  critical: 2
  warning: 3
  info: 2
  total: 7
status: resolved
resolution:
  fixed: [CR-02, WR-01, WR-02, "verifier:FwRibbonMap-array-size"]
  rejected: [CR-01]
  accepted_low: [WR-03, IN-01, IN-02]
  resolution_commit: pending
---

# Phase 02: Code Review Report

**Reviewed:** 2026-06-13
**Depth:** deep
**Files Reviewed:** 13
**Status:** issues_found

## Summary

Phase 02 introduces the FreeWorks CommandManager ribbon: `FwRibbon` (a `QTabWidget`-based ribbon shell), `FwRibbonMap` (the curated command table), `FwRibbonContext` (the sketch-edit contextual tab switcher), `FwLayout` (mount/unmount/chrome-hide lifecycle), plus tests. The architecture is sound — the App/Gui separation is respected, no Sketcher headers leak in, the QPointer-based ribbon guard is correctly placed, the suppress-flag logic covers all production rebuild paths, and the unmount-before-teardown ordering in `unmountRibbon()` is correct.

Two blockers are found:

1. **All three new test files include FreeWorks headers via the path `<src/Gui/FreeWorks/...>`, which cannot resolve under the current CMake include configuration.** With the repository root never added to the include path, this path is broken and the new test targets will not compile. This is a build-breaking defect.

2. **`m_suppressTabStateSave` is a plain `bool` set manually in `buildFromCuratedMap()` and `buildAutoDerived()`.** If anything between the `= true` line and the `= false` line throws (Qt allocation, addTo, etc.), the flag is left permanently `true`, making all future `currentChanged`-driven saves silent no-ops for the lifetime of the ribbon. RAII is needed.

Three warnings follow that are below build-breaking but should be fixed before the next phase.

---

## Critical Issues

### CR-01: Test `#include <src/Gui/FreeWorks/...>` paths are unresolvable — tests will not compile

**Files:**
- `tests/src/Gui/FwRibbon.cpp:13-14`
- `tests/src/Gui/FwRibbonWidget.cpp:22-24`
- `tests/src/Gui/FwTestGuiBootstrap.h` (inherits same issue indirectly)

**Issue:**
All three new test files include FreeWorks implementation headers using angle-bracket paths prefixed with `src/`:

```cpp
// FwRibbon.cpp:13-14
#include <src/Gui/FreeWorks/FwRibbonContext.h>
#include <src/Gui/FreeWorks/FwRibbonMap.h>

// FwRibbonWidget.cpp:22-24
#include <src/Gui/FreeWorks/FwLayout.h>
#include <src/Gui/FreeWorks/FwRibbon.h>
#include <src/Gui/FreeWorks/FwRibbonContext.h>
```

The directory `tests/src/Gui/FreeWorks/` does not exist. The CMake include path for the `Gui_tests_run` and `FwRibbonWidget_Tests_run` targets contains (via `FreeCADGui` PUBLIC propagation) `src/Gui/` and `src/`, but NOT the repository root. With `src/` in the path, `<src/Gui/FreeWorks/FwRibbon.h>` resolves to `src/src/Gui/FreeWorks/FwRibbon.h`, which does not exist. The `tests/CMakeLists.txt` `include_directories(${CMAKE_CURRENT_SOURCE_DIR})` adds `tests/`, not the repo root — confirming no path exists to resolve these includes.

The `<src/App/InitApplication.h>` pattern used in the same test files WORKS because `tests/src/App/InitApplication.h` is a locally committed test-helper file; there is no analogous mirror of the FreeWorks production headers under `tests/`.

**Fix (two valid approaches — pick one):**

Option A — Change the includes to use the correct path (recommended; relies on the `src/` PUBLIC include from `FreeCADGui`):

```cpp
// FwRibbon.cpp
#include <Gui/FreeWorks/FwRibbonContext.h>
#include <Gui/FreeWorks/FwRibbonMap.h>

// FwRibbonWidget.cpp
#include <Gui/FreeWorks/FwLayout.h>
#include <Gui/FreeWorks/FwRibbon.h>
#include <Gui/FreeWorks/FwRibbonContext.h>
```

Option B — Add `${CMAKE_SOURCE_DIR}` to `Gui_tests_run` and `FwRibbonWidget_Tests_run` in `tests/src/Gui/CMakeLists.txt`:

```cmake
target_include_directories(Gui_tests_run PRIVATE ${CMAKE_SOURCE_DIR})
# and in setup_qt_test or per-target for FwRibbonWidget
```

Option A is less invasive (no CMake change) and consistent with how `<Gui/Application.h>` is already included in these same files.

---

### CR-02: `m_suppressTabStateSave` is not exception-safe — stuck `true` permanently suppresses all tab-state persistence

**File:** `src/Gui/FreeWorks/FwRibbon.cpp:221-253, 259-295`

**Issue:**
Both `buildFromCuratedMap()` and `buildAutoDerived()` use a manual bool-flag pattern:

```cpp
m_suppressTabStateSave = true;   // line 221 / 259
clearTabs();
// ... Qt/FreeCAD operations that can throw ...
m_suppressTabStateSave = false;  // line 252 / 294
restoreTabState();
```

If anything between the two assignments throws — a Qt allocation, a `getCommandByName` result walking into a bad `addTo()` codepath, or any other exception from within `clearTabs()`, `tabPageForName()`, `panelForName()`, or `cmd->addTo()` — `m_suppressTabStateSave` is left permanently `true`. After that, every `currentChanged` emission (including every real user tab click) calls `saveTabState()` which returns immediately without saving. The user's tab selection is silently never persisted for the ribbon's remaining lifetime.

`clearTabs()` in particular emits `currentChanged` during `removeTab(0)` calls; Qt widget allocation can fail. The bug is latent rather than routinely triggered, but it corrupts a user-visible invariant invisibly.

**Fix:** Use RAII to guarantee the flag is reset:

```cpp
// Add a local scope guard at the top of each build function:
struct SuppressGuard {
    bool& flag;
    explicit SuppressGuard(bool& f) : flag(f) { flag = true; }
    ~SuppressGuard() { flag = false; }
};

void FwRibbon::buildFromCuratedMap()
{
    SuppressGuard guard(m_suppressTabStateSave);
    clearTabs();
    // ... rest of build ...
    // restoreTabState() stays at end; guard resets the flag on any exit path.
    restoreTabState();
}
```

Or use C++20 `std::scope_exit` if available, or a simple lambda with the RAII wrapper.

---

## Warnings

### WR-01: `FwWorkbench.h` includes `<Gui/Workbench.h>` BEFORE `"PreCompiled.h"` — breaks non-PCH builds

**File:** `src/Gui/FreeWorks/FwWorkbench.h:27,29`

```cpp
#include <Gui/Workbench.h>   // line 27 — BEFORE PreCompiled.h
// ...
#include "PreCompiled.h"     // line 29
```

FreeCAD convention (enforced by `.clang-format` and the PCH setup) requires `PreCompiled.h` to be the first include in every FreeWorks header so that `FCConfig.h`/`FCGlobal.h` (and on Windows `<windows.h>`) are established before any other header. On PCH-enabled builds the compiler injects the PCH before all TU includes, masking the issue; on non-PCH builds (e.g. CMake option `FREECAD_USE_PCH=OFF`, CI sanitizer builds), the `FC_OS_WIN32` and `GuiExport` defines in `PreCompiled.h` are not set when `Gui/Workbench.h` is parsed.

**Fix:**

```cpp
// FwWorkbench.h — put PreCompiled.h first:
#pragma once

#include "PreCompiled.h"

#include <Gui/Workbench.h>
```

---

### WR-02: `FwLayout::mountRibbon()` instantiates a new `FwRibbon` with no parent before any null-checks pass — leaks on early exit

**File:** `src/Gui/FreeWorks/FwLayout.cpp:160-176`

```cpp
auto* ribbon = new FwRibbon();       // line 160 — allocated unconditionally
ribbon->buildFromCuratedMap();
// ... curatedHasPanels check ...
if (!curatedHasPanels) {
    // wb nullable check here
    if (wb != nullptr) {
        ribbon->buildAutoDerived(wb->getToolbarItems());
    }
}

if (existing != nullptr) {            // line 178 — reuse path
    // ...
    existing->addWidget(ribbon);      // ribbon reparented — OK
    return;
}

auto* wrapper = new Gui::ToolBar(mw); // line 203
// ...
wrapper->addWidget(ribbon);          // ribbon reparented — OK
mw->addToolBar(Qt::TopToolBarArea, wrapper);
```

The early-return guard (`if (mw == nullptr) return;`) fires at line 145, BEFORE `ribbon` is allocated. There is no early return between lines 160 and 194 where `ribbon` could be orphaned. However, there is a subtle ownership gap: `ribbon` is created with no parent at line 160. If `existing->addWidget(ribbon)` or `mw->addToolBar()` throw (OOM), `ribbon` leaks because no parent has taken ownership yet, and there is no RAII handle. The risk is low in practice but real on memory-constrained targets.

**Fix:** Use a `std::unique_ptr<FwRibbon>` and release ownership only when it is adopted by the toolbar:

```cpp
auto ribbon = std::make_unique<FwRibbon>();
ribbon->buildFromCuratedMap();
// ... curated check ...

if (existing != nullptr) {
    // ...
    existing->addWidget(ribbon.release());  // Qt takes ownership
    return;
}

auto* wrapper = new Gui::ToolBar(mw);
wrapper->addWidget(ribbon.release());      // Qt takes ownership
```

---

### WR-03: `FwTestGuiBootstrap.h` silently swallows ALL baseline module import failures — test failures appear as command-resolution failures, not load failures

**File:** `tests/src/Gui/FwTestGuiBootstrap.h:89-103`

```cpp
auto tryImport = [](const char* module) {
    try {
        Base::Interpreter().runString((std::string("import ") + module).c_str());
    }
    catch (const Base::Exception&) {
        // An optional module may be absent in a trimmed build; baseline
        // modules (PartDesignGui/SketcherGui/MeasureGui) are expected present.
    }
};

tryImport("PartDesignGui");   // BASELINE — always expected
tryImport("SketcherGui");     // BASELINE — always expected
tryImport("MeasureGui");      // BASELINE — always expected
tryImport("MatGui");          // optional
```

The comment acknowledges that `PartDesignGui`, `SketcherGui`, and `MeasureGui` are "expected present", but the catch block swallows their failures identically to the truly optional `MatGui`. If a baseline module fails to load (typo in import, build regressions, Python path issue), `done=true` is still set and the resolution tests `EXPECT_NE(nullptr, resolve(...))` fail with the misleading message "curated row did not resolve", hiding the actual root cause (module never loaded).

**Fix:** Distinguish baseline from optional modules and assert on baseline failures:

```cpp
auto tryImportRequired = [](const char* module) {
    try {
        Base::Interpreter().runString((std::string("import ") + module).c_str());
    }
    catch (const Base::Exception& e) {
        // A baseline module failing to load is a fatal bootstrap error.
        ADD_FAILURE() << "Required module '" << module
                      << "' failed to import: " << e.what();
    }
};

auto tryImportOptional = [](const char* module) {
    try {
        Base::Interpreter().runString((std::string("import ") + module).c_str());
    }
    catch (const Base::Exception&) {}
};

tryImportRequired("PartDesignGui");
tryImportRequired("SketcherGui");
tryImportRequired("MeasureGui");
tryImportOptional("MatGui");
```

---

## Info

### IN-01: `addTabFromCommandIds()` does not set `m_suppressTabStateSave` — may clobber persisted tab index

**File:** `src/Gui/FreeWorks/FwRibbon.cpp:99-132`

`addTabFromCommandIds()` calls `addTab()` which triggers `currentChanged` (the first tab added becomes current), which in turn calls `saveTabState()`. This writes index 0 to the `ParameterGrp` key, clobbering any previously persisted user selection. This method is only used in tests (not in the production `mountRibbon()` path which uses `buildFromCuratedMap()` / `buildAutoDerived()`), but the inconsistency creates surprising test interactions if `saveTabState()` / `restoreTabState()` round-trip tests run before widget tests that use `addTabFromCommandIds()`.

**Fix:** Add the suppress guard at the start of `addTabFromCommandIds()` (consistent with the other build methods), or document it as a test-only primitive that intentionally skips persistence.

---

### IN-02: `FwRibbonContext::connect()` updates state machine (`previousIndex_`) even when `ribbon_` is null

**File:** `src/Gui/FreeWorks/FwRibbonContext.cpp:99-109`

In the `signalInEdit` lambda:

```cpp
const TabAction action = decideOnEnter(isSketchType(typeName),
                                       ribbon_ ? ribbon_->currentIndex() : 0);
if (action == TabAction::SwitchToSketch && ribbon_) {
    ribbon_->setCurrentTab(kSketchTabName);
}
```

If `ribbon_` is null when the signal fires (possible during transition between mount/unmount in an unusual shutdown sequence), `decideOnEnter` is called with `currentIndex=0` as a fallback. If `isSketch=true`, `contextActive_` becomes `true` and `previousIndex_=0` is stashed. If `ribbon_` is subsequently rebound and a `resetEdit` fires, the ribbon restores to index 0 regardless of where the user actually was. In normal operation this window is closed by `unmountRibbon()` calling `s_ribbonContext.reset()` first (which disconnects), but if the context is ever disconnected without resetting (e.g., future refactor), this silent state corruption would surface.

**Fix:** Add an early-exit guard at the top of the inEdit lambda:

```cpp
inEditConn_ = app->signalInEdit.connect(
    [this](const Gui::ViewProviderDocumentObject& vp) {
        if (!ribbon_) {
            return;   // no ribbon — skip state machine mutation entirely
        }
        const std::string typeName = vp.getTypeId().getName();
        const TabAction action = decideOnEnter(isSketchType(typeName),
                                               ribbon_->currentIndex());
        if (action == TabAction::SwitchToSketch) {
            ribbon_->setCurrentTab(QString::fromUtf8(kSketchTabName));
        }
    });
```

---

_Reviewed: 2026-06-13_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: deep_

---

## Resolution Log (orchestrator, 2026-06-13)

Findings were independently re-verified against source before action (no blind apply).

| ID | Disposition | Evidence / Fix |
|----|-------------|----------------|
| **CR-01** (test include `<src/Gui/FreeWorks/...>` won't resolve) | **REJECTED — false positive** | `<src/...>` is the established FreeCAD test-include convention: **58 upstream test files** use it (e.g. `tests/src/App/Property.cpp:42` → `#include <src/App/InitApplication.h>`), as does the already-committed Phase-1 `tests/src/Gui/FwWorkbench.cpp:16`. The repo root is on the test include path. The fork's tests correctly match this; the suggested `<Gui/FreeWorks/...>` change would *break* consistency. No change made. |
| **CR-02** (suppress flag stuck-true on throw → silent loss of tab persistence) | **FIXED** | `FwRibbon.cpp` `buildFromCuratedMap()`/`buildAutoDerived()` now use `QScopedValueRollback<bool> suppressGuard(m_suppressTabStateSave, true)` (function-scoped RAII; rolls back on exit incl. exception). Manual `= true`/`= false` assignments removed. |
| **WR-01** (`<Gui/Workbench.h>` before `"PreCompiled.h"` → non-PCH break) | **FIXED** | `FwWorkbench.h` now includes `"PreCompiled.h"` first, matching the sibling `FwLayout.h`. |
| **WR-02** (`new FwRibbon()` leaks on a throw before adoption) | **FIXED** | `FwLayout::mountRibbon()` now holds the ribbon in `std::make_unique<FwRibbon>()` and `release()`s it at each `QToolBar::addWidget()` ownership-transfer site. |
| **verifier: FwRibbonMap `std::array<…,56>` vs 62 rows** | **FIXED (hard compile error)** | `FwRibbonMap.cpp` switched to `std::to_array<FwRibbonRow>({...})` — size is now deduced, eliminating the count-drift bug class entirely (important: this fork has no local build to catch an ill-formed `std::array<T,N>`). |
| **WR-03** (test bootstrap swallows REQUIRED module-import failures) | **ACCEPTED — low** | Test-diagnostics quality only; not a happy-path correctness bug. Changing to `ADD_FAILURE` alters test behavior and is better validated with a live build. Tracked for a future test-hardening pass. |
| **IN-01** (`addTabFromCommandIds` lacks the suppress guard) | **ACCEPTED — low** | Test-only construction helper; not on a production rebuild path. |
| **IN-02** (`signalInEdit` lambda mutates state when `ribbon_` is null) | **ACCEPTED — low** | Current unmount-before-teardown ordering makes this a future-refactor trap, not a live bug (reviewer's own assessment). |

**Net:** all compile-breaking and silent-correctness findings resolved; one finding rejected as a false positive with evidence; three info/test-quality items consciously deferred. Re-ran `tools/fw-string-leak-grep.sh` after fixes — clean.
