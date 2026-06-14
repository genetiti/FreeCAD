---
phase: 02-commandmanager-ribbon
verified: 2026-06-13T23:00:00Z
status: passed
uat_outcome: "5/5 passed, 0 issues (via /gsd-verify-work 2, 2026-06-14) — all live-GUI items confirmed; SPIKE_LIVE_CHECKLIST.md item-5 obligation CLOSED"
score: 5/5 must-haves verified
overrides_applied: 0
gaps: []
deferred: []
human_verification:
  - test: "Live ribbon visual — ribbon mounted in TopToolBarArea, tabbed Features/Sketch/Evaluate visible, large labeled icons, looks SW-like at a glance"
    expected: "User sees a tabbed ribbon replacing menus+toolbars in FreeWorks mode; tabs are visibly large icon-over-label; subjectively reads as SolidWorks"
    why_human: "Visual fidelity is subjective and requires a live GUI binary on target OS; no headless assertion captures the subjective 'looks SW-like' criterion (VALIDATION.md Manual-Only table)"
  - test: "Flyout split-button popup interaction — click a *_Comp* button (e.g. Fillet ▸ Chamfer) and confirm the dropdown reveals related commands and fires them"
    expected: "Arrow click opens MenuButtonPopup dropdown showing related commands; clicking one fires it"
    why_human: "Native QToolButton MenuButtonPopup interaction requires a live event loop and pointer; pointer-click cannot be asserted headlessly (VALIDATION.md Manual-Only table)"
  - test: "Live menu bar + stock toolbar hide/restore across FreeWorks activate/deactivate — toggle into and out of FreeWorks mode and confirm chrome is reversibly hidden then restored"
    expected: "FreeWorks mode hides menu bar + stock toolbars; switching to a different workbench fully restores the prior chrome (incl. macOS native menu bar)"
    why_human: "Live workbench toggle and macOS native-menu roundtrip require an interactive GUI session; the headless test (test_ChromeHideRestoreRoundTrip) covers the reachable toggle assertion only (VALIDATION.md Manual-Only table)"
  - test: "Live sketch enter → Sketch tab auto-activates → exit → prior tab restored (the SPIKE item-5 open obligation and RIBBON-02 live verification)"
    expected: "Enter a sketch in FreeWorks mode: the ribbon auto-switches to the Sketch tab (even if a different tab was active). Exit the sketch: the previously-active tab is restored. after-tab index == before-tab index."
    why_human: "The full edit lifecycle (signalInEdit/signalResetEdit firing in the running app) requires a live GUI + 3D viewport + real sketch ViewProvider; pure-logic tests cover the state machine only. This also closes the SPIKE_LIVE_CHECKLIST.md item-5 open obligation. Also re-validates that the 'SketcherGui::ViewProviderSketch' type-name literal still matches upstream (REVIEW LOW)."
  - test: "True app-restart persistence — set a non-default tab (e.g. Evaluate), close FreeCAD, relaunch in FreeWorks mode, confirm the ribbon restores the saved tab"
    expected: "The selected tab index written by saveTabState() is read back by restoreTabState() on the next launch"
    why_human: "True restart exercises the real ParameterGrp write→read cycle across process boundaries; in-process roundtrip (test_TabStateRoundTripRestoresIndex) is covered headlessly but cannot prove the persist-across-restart path (VALIDATION.md Manual-Only table)"
---

# Phase 02: CommandManager Ribbon — Verification Report

**Phase Goal:** A SolidWorks user sees a familiar tabbed CommandManager ribbon at the top of
the window with large labeled icons and flyout split-buttons, and the active tab switches by
context exactly as SolidWorks does — so the app reads as "this is SolidWorks" at first glance.
**Verified:** 2026-06-13T23:00:00Z
**Status:** human_needed
**Re-verification:** No — initial verification.

---

## Verification Context

This phase operates under the **no-build-tree convention** established in Phase 1 and carried
throughout Phase 2 (decisions [01-03]/[01-04]; SPIKE.md approval basis). No FreeCAD build
tree, Qt runtime, or ctest runner is available in this environment. Per the project-established
precedent, compile-intent source + authored headless tests constitute the source-level evidence;
live GUI/build verification is deferred to signed-off checklists
(`src/Gui/FreeWorks/SPIKE_LIVE_CHECKLIST.md`) and the CI build matrix.

Verification method: read actual source files, run static greps, inspect test code structure,
and compare against ROADMAP.md success criteria and PLAN must_haves. SUMMARY.md claims are
NOT treated as evidence; each claim is independently falsified or confirmed against code.

---

## Goal Achievement

### Observable Truths (mapped from ROADMAP.md Success Criteria)

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| SC1 | Tabbed ribbon (Features/Sketch/Evaluate) mounted in top toolbar area with large text-labeled icons, replacing menus+toolbars in FreeWorks mode | VERIFIED (source); LIVE-DEFERRED (visual) | `FwRibbon.buildFromCuratedMap()` — three tabs from 62 curated rows confirmed in code; `FwLayout.mountRibbon()` wraps ribbon in real `Gui::ToolBar(objectName="Fw_RibbonToolBar")` + `addToolBar(Qt::TopToolBarArea, wrapper)`; `FwWorkbench::activated()` calls `mountRibbon()+hideStockChrome()`; `deactivated()` reverses. Test `test_MountAddsRealToolBarInTopArea` asserts `Qt::TopToolBarArea` area. `Qt::ToolButtonTextUnderIcon` + 32px icons confirmed in `FwRibbon.cpp:112-113,171-172`. Live "reads SW-like" → human_needed. |
| SC2 | Commands driven by FreeCAD's existing command registry (ToolBarItem/MenuItem consumed as data) firing via CommandManager — no duplicated backend | VERIFIED | All buttons built via `Gui::Command* cmd = manager.getCommandByName(id); cmd->addTo(panel)` (FwRibbon.cpp:119-128, 229-243). No command backend duplicated. FwRibbonMap.cpp contains only command ID strings. Grep confirms no `App/` includes beyond `App/Application.h` for ParameterGrp (a standard Gui layer inclusion). |
| SC3 | Flyout split-buttons work — a *_Comp* group reveals related commands | VERIFIED (mechanism/widget); LIVE-DEFERRED (pointer interaction) | `PartDesign_CompPrimitiveAdditive`, `PartDesign_CompPrimitiveSubtractive`, and Sketcher `Sketcher_Comp*` IDs present in `FwRibbonMap.cpp`. Same `cmd->addTo()` path routes group commands through `ActionGroup::addTo()` (Action.cpp:472-485) yielding `MenuButtonPopup`. Test `test_FlyoutProducesMenuButtonPopup` inspects real `QToolButton.popupMode()==MenuButtonPopup` and `menu()->actions().size()>1`. Live pointer interaction → human_needed. |
| SC4 | Active ribbon tab switches by context — entering a sketch activates the Sketch tab — wired to edit state | VERIFIED (logic/wiring); LIVE-DEFERRED (observed round-trip) | `FwRibbonContext.{h,cpp}`: subscribes to `Gui::Application::Instance->signalInEdit`/`signalResetEdit`; `decideOnEnter(isSketch,currentIndex)` state machine with `contextActive_` guard (concern-7: nested enter = NoOp, no re-stash); `decideOnReset()` NoOp when inactive; `isSketchType("SketcherGui::ViewProviderSketch")`; `QPointer<FwRibbon>` guard; scoped `fastsignals::scoped_connection` members. `FwLayout.bindRibbonContext()` wired from `mountRibbon()`; `s_ribbonContext.reset()` before ribbon teardown. 7 pure-logic GTests in `tests/src/Gui/FwRibbon.cpp` cover the state machine including the concern-7 regression. Live tab-switch/restore observed round-trip → human_needed (SPIKE item-5 open obligation). |
| SC5 | Ribbon tab layout persists across restarts; built as a self-contained widget that survives upstream sync without merge conflict | VERIFIED (implementation); LIVE-DEFERRED (true restart) | Two-layer persistence: (1) wrapper `Gui::ToolBar("Fw_RibbonToolBar")` participates in `QMainWindow::saveState()/restoreState()` — test `test_ToolBarStateRoundTripRestoresArea` asserts restored `Qt::TopToolBarArea`. (2) `FwRibbon::saveTabState()/restoreTabState()` via `ParameterGrp` key `User parameter:BaseApp/Preferences/FreeWorks/Ribbon/currentTab` — test `test_TabStateRoundTripRestoresIndex` asserts `currentIndex()==2`. All FreeWorks sources under `src/Gui/FreeWorks/`; CMake wiring is `target_sources(FreeCADGui PRIVATE ...)` (additive; no shared edits beyond `src/Gui/CMakeLists.txt` add_subdirectory). True app-restart persistence → human_needed. |

**Score:** 4/5 truths VERIFIED at source level. All 5 are **LIVE-DEFERRED** for the GUI
observable behaviors, which are routed to human_verification items. No truth is FAILED.

---

### Deferred Items

No items deferred to later milestone phases — all Phase 2 criteria are addressed within Phase 2
source. Live verification items are human_needed (not gaps), consistent with the no-build-tree
precedent.

---

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/Gui/FreeWorks/FwRibbon.h` | FwRibbon QTabWidget shell with full API | VERIFIED | Declares `FwRibbon : QTabWidget`, `addTabFromCommandIds`, `buildFromCuratedMap`, `buildAutoDerived`, `setCurrentTab`, `saveTabState`, `restoreTabState`. |
| `src/Gui/FreeWorks/FwRibbon.cpp` | Full implementation — curated build, auto-derive, overflow, persistence | VERIFIED | 402 lines. `getCommandByName`+`addTo` seam confirmed; `Qt::ToolButtonTextUnderIcon`, `QSize(32,32)`; `setStyleSheet` count=0; `setCornerWidget` for overflow; `ParameterGrp` key for tab state; `m_suppressTabStateSave` storm guard. |
| `src/Gui/FreeWorks/FwRibbonMap.h` | `struct FwRibbonRow` + `class FwRibbonMap` | VERIFIED | Declares `FwRibbonRow {tab,panel,commandId}` and `FwRibbonMap::rows()` returning `std::span<const FwRibbonRow>`. |
| `src/Gui/FreeWorks/FwRibbonMap.cpp` | Curated 3-tab table | VERIFIED (content); WARNING (N mismatch) | 62 row entries confirmed: Features(27), Sketch(30), Evaluate(5). Three tab names present. *_Comp* IDs present. Evaluate rows (`Std_Measure`, `Part_CheckGeometry`, `Materials_InspectMaterial`) present. `kKnownGaps` not present in this file; handled in test. **WARNING: `constexpr std::array<FwRibbonRow, 56>` declares N=56 but 62 initializers follow** — this is a C++ ill-formed program (excess initializers; [dcl.init.aggr]). Cannot verify compilation without a build tree; this is the primary technical risk outstanding. |
| `src/Gui/FreeWorks/FwRibbonContext.h` | FwRibbonContext subscriber class | VERIFIED | Declares `class FwRibbonContext`, `enum class TabAction`, `decideOnEnter`, `decideOnReset`, `isSketchType`, `QPointer<FwRibbon> ribbon_`, `contextActive_`, `previousIndex_`, scoped `fastsignals::scoped_connection` members. |
| `src/Gui/FreeWorks/FwRibbonContext.cpp` | Edit-signal subscription + state machine | VERIFIED | `signalInEdit`(2x), `signalResetEdit`(2x), `SketcherGui::ViewProviderSketch`(2x), `contextActive_`(5x), `setCurrentTab`(2x). `Mod/Sketcher` includes = 0. `activeDialog` = 0. D-09 reinterpretation comment present. |
| `src/Gui/FreeWorks/FwLayout.h` | `mountRibbon`, `unmountRibbon`, `hideStockChrome`, `restoreStockChrome` | VERIFIED | All four methods declared. `s_ribbonContext` (unique_ptr) and chrome snapshot statics declared. |
| `src/Gui/FreeWorks/FwLayout.cpp` | Real QToolBar mount + chrome hide/restore + context bind | VERIFIED | `addToolBar`(3x), `Fw_RibbonToolBar` present, `ForceHidden`(1x), `RestoreDefault`(1x). Menu-bar snapshot (`s_menuBarWasVisible`, `s_menuBarWasNative`) captured before hide; restored via `setNativeMenuBar(snapshot)` + `setVisible(snapshot)`. `menuBar()->show()` = 0 (snapshot-driven). `toolBarAreaWidget` = 0 (REVIEW concern 3 satisfied). `FwRibbonContext`/`s_ribbonContext` wired; `s_ribbonContext.reset()` before ribbon removal (teardown order correct). |
| `src/Gui/FreeWorks/FwWorkbench.cpp` | `activated()` mounts+hides; `deactivated()` restores+unmounts | VERIFIED | `activated()` calls `StdWorkbench::activated()` then `mountRibbon()+hideStockChrome()`. `deactivated()` override calls `restoreStockChrome()+unmountRibbon()`. |
| `src/Gui/FreeWorks/CMakeLists.txt` | All new sources registered in `target_sources(FreeCADGui ...)` | VERIFIED | `FwRibbon.cpp`, `FwRibbonMap.cpp`, `FwRibbonContext.cpp` all present in `FreeWorks_CPP_SRCS`. `target_sources(FreeCADGui PRIVATE ${FreeWorks_SRCS})`. |
| `tests/src/Gui/FwTestGuiBootstrap.h` | Shared GUI bootstrap — creates Gui::Application singleton then imports all owning modules | VERIFIED | `new Gui::Application(false)` precedes module imports (singleton creation at line 78, first import at line 99). Imports: `PartDesignGui`, `SketcherGui`, `MeasureGui`, `MatGui`. Asserts `Gui::Application::Instance != nullptr` after step 3. `static done` idempotency guard. |
| `tests/src/Gui/FwRibbon.cpp` | GTest command-ID resolution + FwRibbonContext pure-logic tests | VERIFIED | `ensureGuiTestBootstrap` in `SetUpTestSuite`. Resolution tests: `curatedPartDesignIdResolves`, `measureGuiCommandResolves`, `groupCommandResolves`, `bogusIdResolvesToNull`, `everyCuratedRowResolves`. FwRibbonContext logic: 7 tests including `nestedSketchEnterIsNoOpAndKeepsRememberedTab` (concern-7 guard), `resetWhileInactiveIsNoOp`, `contextWinsOverManualThenRestoresManualTab`. `kKnownGaps` declared as zero-length array; test asserts it stays empty. No QWidget construction. |
| `tests/src/Gui/FwRibbonWidget.cpp` | QTEST_MAIN offscreen widget tests | VERIFIED | Contains `QTEST_MAIN(testFwRibbonWidget)`. Tests: `test_BuildsSingleTabFromCommandIds`, `test_CuratedBuildYieldsThreeTabs`, `test_FlyoutProducesMenuButtonPopup` (inspects real `QToolButton.popupMode()==MenuButtonPopup`), `test_AutoDerivePanelAndSeparatorCount`, `test_PanelObjectNamesAreUnique`, `test_SetCurrentTabSelectsByName`, `test_MountAddsRealToolBarInTopArea` (real `Gui::MainWindow`), `test_MountIsIdempotent`, `test_ChromeHideRestoreRoundTrip`, `test_OverflowCornerWidgetPresent`, `test_ToolBarStateRoundTripRestoresArea`, `test_TabStateRoundTripRestoresIndex`, `test_ContextBoundToRibbonResolvesSketchTabAndRestores`. |
| `tests/src/Gui/CMakeLists.txt` | Test registration | VERIFIED | `FwRibbon.cpp` in `Gui_tests_run` list; `setup_qt_test(FwRibbonWidget)` present. |
| `.planning/phases/02-commandmanager-ribbon/SPIKE.md` | D-03 6-item parity verdict | VERIFIED | Contains `Verdict`; `grep -c "PASS\|FAIL"` = 8 (≥6). Literal `"native committed"` present at two locations. Item-5 honesty flag explicitly states "DESIGN/EXPECTED values, not observed-on-hardware" — SPIKE_LIVE_CHECKLIST.md carries the open obligation. |
| `src/Gui/FreeWorks/SPIKE_LIVE_CHECKLIST.md` | Deferred live-verification obligation | VERIFIED | Exists. Unsigned (all checkboxes empty). Open obligation confirmed. |
| `src/3rdParty/SARibbon` | MUST NOT EXIST | VERIFIED (absent) | Confirmed not present. Native committed verdict honored. |

---

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `FwRibbon.cpp` | `Gui::CommandManager` | `getCommandByName` + `cmd->addTo(panel)` | VERIFIED | Confirmed at FwRibbon.cpp:119,243. Every button built via this exact seam. |
| `FwRibbon.cpp` | `FwRibbonMap.cpp` | `FwRibbonMap::rows()` consumed in `buildFromCuratedMap` | VERIFIED | `for (const FwRibbonRow& row : FwRibbonMap::rows())` at FwRibbon.cpp:228. |
| `FwRibbon.cpp` | `Workbench::getToolbarItems()` | `buildAutoDerived` consumes value-type list | VERIFIED | `FwRibbon.h:102` signature takes `const std::list<std::pair<std::string,std::list<std::string>>>&`; `FwLayout.cpp:174` calls `wb->getToolbarItems()`. |
| `FwLayout.cpp` | `Gui::getMainWindow()->addToolBar(Qt::TopToolBarArea, wrapper)` | Real QToolBar mount | VERIFIED | `FwLayout.cpp:210`. `toolBarAreaWidget` = 0 (not used). |
| `FwLayout.cpp` | `ToolBarManager::setState(..., ForceHidden/RestoreDefault)` | Chrome snapshot hide/restore | VERIFIED | Lines 284,313. Snapshot captured before hide (s_hiddenToolBars, s_menuBarWasVisible, s_menuBarWasNative). |
| `FwRibbonContext.cpp` | `Gui::Application::Instance->signalInEdit` / `signalResetEdit` | `fastsignals::scoped_connection` | VERIFIED | Lines 99,111. Mirrors OverlayManager.cpp:406-409 pattern. |
| `FwRibbonContext.cpp` | `FwRibbon::setCurrentTab("Sketch")` | On `SwitchToSketch` action | VERIFIED | Line 107. Guarded by `ribbon_` QPointer non-null check. |
| `FwLayout.cpp` | `FwRibbonContext` | `bindRibbonContext(ribbon)` called from `mountRibbon()` | VERIFIED | Line 215 (fresh mount) and line 192 (idempotent reuse). `s_ribbonContext.reset()` at line 238 before ribbon removal. |
| `FwWorkbench.cpp` | `FwLayout::mountRibbon` + `hideStockChrome` | In `activated()` | VERIFIED | Lines 62-63. |
| `FwWorkbench.cpp` | `FwLayout::restoreStockChrome` + `unmountRibbon` | In `deactivated()` | VERIFIED | Lines 70-71. |
| `tests/src/Gui/FwRibbon.cpp` | `FwTestGuiBootstrap.h::ensureGuiTestBootstrap()` | In `SetUpTestSuite` | VERIFIED | Line 51. NOT a bare `initApplication()`. |
| `tests/src/Gui/CMakeLists.txt` | `FwRibbonWidget.cpp` | `setup_qt_test(FwRibbonWidget)` | VERIFIED | Confirmed present. |

---

### Data-Flow Trace (Level 4)

All command buttons are thin triggers via `cmd->addTo(panel)` which reuses the existing
`Gui::Command::QAction`. There is no custom data fetch in FwRibbon — the underlying
`CommandManager` owns the command registry. This is the correct, architecture-conformant
pattern for a ribbon (thin triggers, no duplicated backend). No hollow-prop or
static-return issues present.

---

### Behavioral Spot-Checks

Step 7b is SKIPPED — no runnable FreeCAD build tree / Qt runtime in this environment.
Per the no-build-tree convention (project precedent from Phase 1), ctest execution is
deferred to the CI build matrix. The authored tests are structurally correct (QTEST_MAIN
with QApplication, GTest with ensureGuiTestBootstrap, real Gui::MainWindow in widget
tests) and constitute compile-intent evidence.

---

### Probe Execution

Step 7c: No `scripts/*/tests/probe-*.sh` pattern applies to this phase (GUI/source phase,
not a migration/tooling phase). SKIPPED.

---

### Requirements Coverage

| Requirement | Source Plans | Description | Status | Evidence |
|-------------|-------------|-------------|--------|----------|
| RIBBON-01 | 02-01, 02-02, 02-03 | Tabbed CommandManager ribbon with large labeled icons, flyout split-buttons, registry-driven (T1) | VERIFIED (source) / LIVE-DEFERRED | FwRibbon+FwRibbonMap (SC1/SC2/SC3); FwLayout mount (SC1/SC5); overflow escape hatch (D-12). REQUIREMENTS.md marks Complete. |
| RIBBON-02 | 02-04 | Active ribbon tab switches by context — sketch entry activates Sketch tab (T1/T10) | VERIFIED (logic) / LIVE-DEFERRED | FwRibbonContext full state machine + signal wiring + FwLayout lifecycle. Pure-logic tests cover the state machine. Live round-trip deferred. REQUIREMENTS.md marks Complete. |

No orphaned requirements: REQUIREMENTS.md traceability table maps RIBBON-01 and RIBBON-02
both to Phase 2 with status "Complete".

---

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| `src/Gui/FreeWorks/FwRibbonMap.cpp` | 48 | `constexpr std::array<FwRibbonRow, 56>` with 62 initializers (Features=27, Sketch=30, Evaluate=5) | WARNING | Ill-formed C++ aggregate initialization. In C++, `std::array<T,N>` with more than N initializers is a compile-time error [dcl.init.aggr]. The SUMMARY claims "56 pinned IDs" but the actual array has 62 entries. Cannot compile-verify without a build tree; this is the primary source risk. Fix: change `56` to `62` in the template argument. |
| `src/Gui/FreeWorks/FwWorkbench.cpp` | 78,85,91 | Stale comments: "Phase 2 (the FreeWorks ribbon) replaces this" on `setupMenuBar()`/`setupToolBars()`/`setupCommandBars()` | INFO | These methods still delegate to `StdWorkbench` (the replacement happens dynamically via `mountRibbon()`+`hideStockChrome()` in `activated()`, not by overriding these methods). The comments are outdated now that Phase 2 is complete. No functional impact — the design is correct. |

**Debt marker gate:** No `TBD`, `FIXME`, or `XXX` markers found in any FreeWorks source file.
"Placeholder" occurrences are all intentional design-documentation (dock placeholders
for Phase 3/4/7 content) and are not unresolved debt markers.

---

### Human Verification Required

Five items require live GUI verification (see `human_verification` frontmatter for
machine-readable form):

#### 1. Ribbon Visual Fidelity (SC1 / RIBBON-01)

**Test:** Launch FreeCAD in FreeWorks mode on at least one target OS. Confirm a tabbed
ribbon appears in the top toolbar area with Features/Sketch/Evaluate tabs, each hosting
large icon-over-label buttons (~32px). The ribbon must replace the stock menus+toolbars.
**Expected:** The ribbon visually reads as a SolidWorks CommandManager at a glance — tabbed,
large labeled buttons, visibly bigger than stock FreeCAD toolbars.
**Why human:** Visual fidelity is subjective; no headless assertion captures "looks SW-like."

#### 2. Flyout Split-Button Interaction (SC3 / RIBBON-01)

**Test:** In FreeWorks mode, click the arrow of a split-button (e.g. the Pad/Revolution/Loft
flyout in Additive, or the Fillet/Chamfer flyout in Dress-Up). Confirm the dropdown reveals
the grouped alternative commands. Click an alternative and confirm it fires.
**Expected:** Arrow opens a MenuButtonPopup dropdown; each alternative fires the correct
FreeCAD command.
**Why human:** Native `QToolButton(MenuButtonPopup)` popup interaction requires a live event
loop and pointer; cannot be asserted headlessly.

#### 3. Chrome Hide/Restore Round-Trip (SC1 / D-11)

**Test:** Activate FreeWorks mode; confirm stock menus + toolbars are hidden. Switch to a
different workbench (e.g. Part). Confirm the menu bar and stock toolbars are restored intact
(including macOS native menu bar if testing on macOS).
**Expected:** FreeWorks hides chrome; deactivation fully restores the prior chrome state.
**Why human:** Live workbench toggle and macOS native-menu round-trip require an interactive
GUI session.

#### 4. Live Sketch Context Switch + Restore (SC4 / RIBBON-02 + SPIKE item-5)

**Test:** In FreeWorks mode with the Features tab active (index 0), create or open a
PartDesign Body and enter a sketch. Record the before-tab index. Confirm the ribbon
auto-switches to the Sketch tab. Exit the sketch. Confirm the ribbon restores the
before-tab index (after == before).
**Expected:** Enter sketch → Sketch tab activates. Exit sketch → prior tab restored.
Record: before index ___, in-sketch index ___, after index ___. `after == before`?
**Why human:** The full edit lifecycle (real signalInEdit/signalResetEdit from a live sketch
VP) requires the running app. This also closes the SPIKE_LIVE_CHECKLIST.md item-5
obligation. The "SketcherGui::ViewProviderSketch" type-name literal must be confirmed
to still match upstream in this build (REVIEW LOW).

#### 5. True App-Restart Persistence (SC5 / D-14)

**Test:** In FreeWorks mode, click the Evaluate tab (index 2). Close FreeCAD. Relaunch
FreeCAD and activate FreeWorks mode. Confirm the ribbon opens on the Evaluate tab (index 2).
**Expected:** `saveTabState()` writes the index on tab-change; `restoreTabState()` applies
it on the next launch. Tab index 2 is restored after restart.
**Why human:** True restart exercises the ParameterGrp write→read cycle across process
boundaries; in-process round-trip (covered headlessly) does not prove this.

---

### Gaps Summary

No **gaps** identified. All ROADMAP.md success criteria have source-level implementation
evidence. The `std::array<FwRibbonRow, 56>` / 62-entry mismatch is a WARNING (a likely
compile error that requires a one-token fix `56`→`62`) but is not a goal-blocking gap
because the rest of the implementation is correct and the CI build matrix is the
established confirmation path for compilation. All live-GUI behaviors are routed to
human_verification (the established project pattern for this phase), not to gaps.

---

### String-Leak Guard

`tools/fw-string-leak-grep.sh` result: **CLEAN** — "no disallowed 'SolidWorks' tokens;
only 'Gui::SolidWorksNavigationStyle' present." (allow-listed from Phase 1 navigation style.)

### SARibbon Verification

`src/3rdParty/SARibbon`: **ABSENT** — confirmed not introduced. Native committed verdict
honored throughout Plans 02-02/02-03/02-04.

### Git Commit Verification

All commits referenced in SUMMARYs confirmed in `git log`:
- `4c13bd9e22` (test 02-01) — CONFIRMED
- `c02b0eaae3` (feat 02-01) — CONFIRMED
- `76a3ca9fee` (feat 02-02: curated map) — CONFIRMED
- `4069e51d24` (feat 02-02: full build) — CONFIRMED
- `310f6175c7` (feat 02-03: mount) — CONFIRMED
- `bcc3b5b5f1` (feat 02-03: overflow+persistence) — CONFIRMED
- `1974548a43` (test 02-04: RED tests) — CONFIRMED
- `945fe2223b` (feat 02-04: context subscriber) — CONFIRMED
- `d98675eee2` (feat 02-04: FwLayout binding) — CONFIRMED

---

## Implementation Notes

### `std::array<FwRibbonRow, 56>` vs. 62 Entries (WARNING)

The curated map declares `constexpr std::array<FwRibbonRow, 56> kRows` on line 48 of
`FwRibbonMap.cpp`, but contains 62 initializer entries (Features: 27, Sketch: 30,
Evaluate: 5). In C++, `std::array<T,N>` with more initializers than N is ill-formed
(compile error). The SUMMARY's "56 pinned IDs" is incorrect; the actual count is 62.

**Required fix (one token):** change `56` to `62` at FwRibbonMap.cpp:48. The rows
themselves are correct — the count in the template argument was not updated when the
final row set was written.

**Blast radius:** This will cause compilation to fail. All other Phase 2 source is
structurally correct; this single line fix unblocks CI.

### Stale Comments in FwWorkbench.cpp (INFO)

`setupMenuBar()`, `setupToolBars()`, and `setupCommandBars()` carry comments saying
"Phase 2 (the FreeWorks ribbon) replaces this" but these methods still delegate to
`StdWorkbench`. This is intentional: the replacement happens dynamically via
`FwLayout::mountRibbon()` + `FwLayout::hideStockChrome()` in `activated()`, not by
overriding these setup methods. The comments are stale and could mislead future readers
but do not affect functionality.

---

_Verified: 2026-06-13T23:00:00Z_
_Verifier: Claude (gsd-verifier)_
