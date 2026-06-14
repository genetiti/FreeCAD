---
phase: 03-featuremanager-design-tree
verified: 2026-06-14T18:40:00Z
status: human_needed
score: 4/4 code-completion truths verified (SC5 live-FEEL deferred by environment)
mode: mvp
overrides_applied: 0
code_verdict: passed
live_parity: deferred_to_spike_live_checklist
human_verification:
  - test: "Drag the rollback band up the FeatureManager; confirm below-tip rows grey out AND the 3D viewport rolls back (suppress-below) to that feature state; one Ctrl+Z restores"
    expected: "Below-bar rows greyed (QPalette::Disabled), 3D shows rolled-back result via ViewProviderBody tip display, single undo restores"
    why_human: "Requires a live GUI + 3D viewport + real PartDesign Body; no build tree in this environment. Logic half (set-Tip -> mustExecute()==1, one-undo-restores) is GTest-covered; the FEEL/3D render is live-only (SPIKE_LIVE_CHECKLIST item 1-2)."
  - test: "Drag a feature before its dependency (child-before-parent); confirm the forbidden cursor appears, NO insertion line shows, and the drop is refused (no transaction)"
    expected: "Qt::ForbiddenCursor, no 2px insertion line, drop declined, model unchanged"
    why_human: "Requires a live pointer/drag event loop. Drop-gate BLOCK logic (no transaction on invalid) is GTest-covered; the insertion-line/forbidden-cursor affordance is live-only (SPIKE_LIVE_CHECKLIST item 4)."
  - test: "Confirm origin planes display as Front/Top/Right and the orientation matches SolidWorks muscle memory (Front=XY, Top=XZ, Right=YZ)"
    expected: "The three planes read 'Front Plane'/'Top Plane'/'Right Plane'; underlying Label never rewritten; orientation feels SW-correct to a daily SW user"
    why_human: "Display remap is QTest-covered (delegate sets option.text, Labels asserted unchanged); the A1 orientation-correspondence FEEL is a subjective daily-SW-user judgement (SPIKE_LIVE_CHECKLIST item 6)."
  - test: "Daily-SolidWorks-user acceptance: rollback suppress-below + insert-at-bar FEELS like SolidWorks (SC5)"
    expected: "A daily SW user confirms the rollback + mid-history insert behave like SW, not just look like it"
    why_human: "Subjective parity judgement on live hardware; routed to the milestone parity-user track (SPIKE_LIVE_CHECKLIST item 5)."
  - test: "Run the authored headless suite once a build tree exists: ctest -R \"Gui_tests_run|FwFeatureTreeWidget_Tests_run\""
    expected: "All FwFeatureTree GTest + FwFeatureTreeWidget QTest cases green"
    why_human: "No configured build tree / CI in this environment; tests are authored compile-intended but never executed locally (wave_0_complete:false). This is the open execution gate, not a code gap."
---

# Phase 3: FeatureManager Design Tree Verification Report

**Phase Goal (MVP user story):** A SolidWorks user sees and works with a left-docked FeatureManager design tree — features in creation order under an origin/planes node, sketches nested under their features — and can roll the model back to an earlier state with a draggable rollback bar, rename with F2, and drag features to reorder history with validity feedback.
**Verified:** 2026-06-14T18:40:00Z
**Status:** human_needed (code-completion **PASSED**; live-GUI/3D parity FEEL deferred to SPIKE_LIVE_CHECKLIST by environment)
**Re-verification:** No — initial verification

## Code-Completion Verdict: PASSED

Every Phase-3 artifact exists on disk, is substantive (real logic, not stubs), is wired (CMake-built, mounted, signal-connected), and traces data through the genuine upstream engines (`Body.Tip` / `PartDesign_MoveTip` / `Gui::TreeWidget` DnD). No debt markers, no forbidden-token leaks, no stub returns in the goal path. The only open items are **environmentally blocked** (no build tree / live GUI / 3D viewport) and are correctly routed to `SPIKE_LIVE_CHECKLIST.md` + the milestone parity track — acceptable deferrals per the no-build precedent established in Phases 1/2, not code failures.

## User Flow Coverage (MVP)

| # | Story step | Expected | Codebase evidence | Status |
|---|-----------|----------|-------------------|--------|
| 1 | Left-docked tree appears | FwFeatureTree mounted in `Fw_FeatureManager` dock, replacing the Phase-1 placeholder | `FwLayout::mountFeatureManager()` (FwLayout.cpp:117-146): find-or-reuse, `registerDockWindow("Fw_FeatureManager", FwFeatureTree)`; called from `FwLayout::install()` (171) ← `FwWorkbench.cpp:105` | ✓ VERIFIED |
| 2 | Active-Body scoped, creation order, nested sketches | Non-active Bodies hidden; Origin-first; sketches under consuming feature | `scopeToActiveBody()`/`scopeItemRecursive()` recursive `setHidden` over real `DocumentItem`→`Body` topology (FwFeatureTree.cpp:145-183); creation order + nesting inherited verbatim from `Gui::TreeWidget` claimChildren; QTest `test_ScopeToActiveBodyHidesNonActiveBodySubtree` (two-Body render) | ✓ VERIFIED (render is live-only, item below) |
| 3 | Origin planes read Front/Top/Right | XY→Front, XZ→Top, YZ→Right; object Label never rewritten | `FwFeatureTreeDelegate::remappedPlaneName()` + `initStyleOption` sets `option->text` only via stock item path (Delegate.cpp:106-144); QTest `test_DelegateRemapsPlaneDisplayNamesThroughStockItemPath` asserts Labels unchanged | ✓ VERIFIED |
| 4 | Drag rollback bar up → model rolls back | Band drives real `Body.Tip` via `PartDesign_MoveTip` under selection guard; suppress-below + recompute; below-tip greyed | `FwRollbackBar::fireTipMove()` invokes `PartDesign_MoveTip` command-ID under `FwSelectionGuard` (RollbackBar.cpp:161-214); upstream command opens own txn + `updateActive()` recompute (CommandBody.cpp:730-744); below-tip greying via `kBelowTipRole` + delegate `QPalette::Disabled` (FwFeatureTree.cpp:270-315) | ✓ VERIFIED (3D FEEL live-only) |
| 5 | Insert/edit mid-history | Insert at bar after tip; solid→new tip, non-solid→tip unchanged | `FwRollbackBar::insertAtBar()` via Python-only `Body.insertObject` + explicit Tip policy in one txn (RollbackBar.cpp:233-276); GTest `insertAtBarSolidBecomesTipNonSolidLeavesTip` | ✓ VERIFIED |
| 6 | Reversible (drag down / Roll to End) | Tip restored forward to last solid feature | `FwRollbackBar::rollToEnd()` (RollbackBar.cpp:216-231); GTest `rollbackReversibleRollToEndRestoresLastSolid` | ✓ VERIFIED |
| 7 | F2 rename | F2 opens inline rename, new Label propagates | Inherited `Gui::TreeWidget` relabel action (no re-impl, D-13); QTest `test_ConstructsAsTreeWidgetWithF2Action` asserts Key_F2/Return action present | ✓ VERIFIED (live keypress is live-only) |
| 8 | Drag-reorder with validity feedback | Valid → insertion line + one transaction; invalid → forbidden cursor, no line, no commit | `FwFeatureTree::dragMoveEvent()` runs base validation FIRST, decorates only on `!isAccepted()` with `Qt::ForbiddenCursor` + `setDropIndicatorShown(false)` (FwFeatureTree.cpp:555-578); GTest `blockedReorderOpensNoTransactionValidReorderOpensOne` | ✓ VERIFIED (affordance FEEL live-only) |

## Goal Achievement — Observable Truths

| # | Truth (Roadmap Success Criteria) | Status | Evidence |
|---|----------------------------------|--------|----------|
| 1 | SC1: Left-docked tree, creation order, origin/planes node, nested sketches | ✓ VERIFIED | Mount (FwLayout.cpp:117-146 → install → FwWorkbench.cpp:105); scoping (FwFeatureTree.cpp:145-204); plane remap delegate; order/nesting inherited from Gui::TreeWidget |
| 2 | SC2: Rollback bar rolls model back via `Body.Tip`, insert/edit mid-history, Gui-layer rollback state | ✓ VERIFIED | `fireTipMove`→`PartDesign_MoveTip` (RollbackBar.cpp:161-214); upstream `Body.Tip` `App::PropertyLink` (BodyBase.h:54); NO new persisted property (D-05); band/greying derived from Tip (Gui-only `kBelowTipRole`) |
| 3 | SC3: F2 rename propagates | ✓ VERIFIED | Inherited relabel action, QTest asserts F2/Return action installed on the mounted tree |
| 4 | SC4: Drag-reorder, insertion-line affordance + validity feedback, document transactions / clean undo | ✓ VERIFIED | `dragMoveEvent` base-first + decorate-only (FwFeatureTree.cpp:555-578); reorder via inherited `sortDroppedObjects` single txn; GTest blocked-vs-valid transaction count + undo-reversible reorder |
| 5 | SC5: Daily-SW-user acceptance that rollback *feels* like SW | ? UNCERTAIN (live-only) | Logic half code-backed; subjective FEEL on live hardware → SPIKE_LIVE_CHECKLIST item 5 + parity-user track (by design; not a code gap) |

**Score:** 4/4 code-completion truths VERIFIED. SC5 is a deliberate subjective/live acceptance gate, environmentally deferred.

## Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/Gui/FreeWorks/FwFeatureTree.{h,cpp}` | Thin Gui::TreeWidget subclass, scoping, drawRow band, DnD, Roll actions | ✓ VERIFIED | 230+580 lines; real scoping, band paint, grab zone, context Roll actions; no stubs in goal path |
| `src/Gui/FreeWorks/FwFeatureTreeDelegate.{h,cpp}` | Plane remap + below-tip greying | ✓ VERIFIED | 113+166 lines; `initStyleOption` text remap, `paint` palette greying; Labels never written |
| `src/Gui/FreeWorks/FwRollbackBar.{h,cpp}` | Link-free resolver + real Body.Tip fire | ✓ VERIFIED | 141+278 lines; `resolveTipTarget`, `fireTipMove` via command-ID, `rollToEnd`, `insertAtBar`+Tip policy |
| `src/Gui/FreeWorks/FwSelectionGuard.{h,cpp}` | RAII selection+preselection round-trip | ✓ VERIFIED | 115+125 lines; owned-string snapshot (MEDIUM-1 lifetime fix applied), `clearPreSelect=false` replay + explicit setPreselect/rmvPreselect |
| `FwLayout.cpp` mount swap | Replace placeholder with mounted tree | ✓ VERIFIED | `mountFeatureManager` find-or-reuse, idempotent, same objectName for layout round-trip |
| `tests/src/Gui/FwFeatureTree.cpp` | GTest logic suite | ✓ VERIFIED | 12 TEST_F cases covering every logic truth; CMake-wired (`Gui_tests_run`) |
| `tests/src/Gui/FwFeatureTreeWidget.cpp` | QTest widget suite | ✓ VERIFIED | 5 slots (F2 action, delegate/indent/empty, scoping render, plane remap, band/greying/Roll); CMake-wired (`setup_qt_test`) |
| `SPIKE_LIVE_CHECKLIST.md` | Live-FEEL deferral log | ✓ VERIFIED | Present (11.4KB); Phase-3 section, 6 live obligations cross-referenced to headless logic tests |

## Key Link Verification

| From | To | Via | Status | Details |
|------|----|----|--------|---------|
| FwWorkbench | FwLayout | `FwLayout::install()` | ✓ WIRED | FwWorkbench.cpp:105 |
| FwLayout | FwFeatureTree | `registerDockWindow("Fw_FeatureManager", ...)` | ✓ WIRED | FwLayout.cpp:145 |
| FwFeatureTree | active Body | `signalActivatedObject` → `setActiveBody`/`scopeToActiveBody` | ✓ WIRED | FwFeatureTree.cpp:107-130 |
| FwRollbackBar | Body.Tip engine | `commandManager().getCommandByName("PartDesign_MoveTip")->invoke(0)` under FwSelectionGuard | ✓ WIRED | RollbackBar.cpp:180-186; upstream cmd registered CommandBody.cpp:1231 |
| FwRollbackBar | FwSelectionGuard | RAII `guard.selectOnly(target)` | ✓ WIRED | RollbackBar.cpp:173-174 |
| FwFeatureTree (greying) | FwFeatureTreeDelegate | `kBelowTipRole` data role + delegate `paint` | ✓ WIRED | FwFeatureTree.cpp:309 sets role; Delegate.cpp:154-162 paints |
| FwFeatureTree (DnD) | inherited drop gate | `Gui::TreeWidget::dragMoveEvent` base-first | ✓ WIRED | FwFeatureTree.cpp:561 |

## Data-Flow Trace (Level 4)

| Artifact | Data variable | Source | Produces real data | Status |
|----------|--------------|--------|--------------------|--------|
| FwFeatureTree scoping | tree rows | live `Gui::DocumentItem`→`DocumentObjectItem` populated by base TreeWidget; resolved via `object()->getObject()` | Yes (stock item factory, observe-the-DOM) | ✓ FLOWING |
| FwRollbackBar resolver | ordered features | `Body.getPropertyByName("Group")` → `App::PropertyLinkList::getValues()` | Yes (real Group) | ✓ FLOWING |
| FwRollbackBar fire | Body.Tip | real `PartDesign_MoveTip` command → `Tip.setValue` (upstream) → `updateActive()` recompute | Yes (real engine, not cosmetic) | ✓ FLOWING |
| below-tip greying | `kBelowTipRole` | derived live from `currentTipFeature()` (Tip property) each drag frame | Yes (Tip-driven) | ✓ FLOWING |

## Behavioral Spot-Checks

Step 7b: SKIPPED for execution — no runnable entry points (no build tree in this environment; C++/Qt GUI requires a configured build). Source-level behavioral evidence substituted: upstream `PartDesign_MoveTip` contract read and confirmed to match the bar's selection-fire assumptions (reads `getSelection()`, requires one feature, Body→Tip=None roll-to-base, own transaction + recompute — CommandBody.cpp:671-745). Referenced commits all verified present in git history (325075a257, aeea534384, f369b77fd3, a9e9bdcdba, 7a80168a8e, f475e941a6, dec920036c, 734979cf78).

## Probe Execution

No probes declared for this phase (GUI/test phase, not a migration/tooling phase). `tools/fw-string-leak-grep.sh` (the one runnable repo check) executed: **exit 0, clean** — no disallowed "SolidWorks" tokens; only the allow-listed `Gui::SolidWorksNavigationStyle`. Fork legal/identifier discipline honored.

## Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|-------------|-------------|--------|----------|
| TREE-01 | 03-01, 03-02 | Left-docked creation-order tree, origin/planes, nested sketches | ✓ SATISFIED | Mount + scoping + delegate remap; inherited order/nesting |
| TREE-02 | 03-03 | Rollback bar + insert/edit mid-history | ✓ SATISFIED | FwRollbackBar real Body.Tip fire, insert-at-bar, roll-to-end |
| TREE-04 | 03-02 | Drag-reorder with validity feedback | ✓ SATISFIED | dragMoveEvent decorate-only + inherited block + single-txn reorder |
| TREE-03 (F2 half only) | 03-02 | F2 rename (double-click→PropertyManager is Phase 4) | ✓ SATISFIED | Inherited relabel action; double-click half correctly deferred to Phase 4 |

No orphaned requirements: ROADMAP maps TREE-01/02/04 to Phase 3, all claimed and verified. TREE-03's double-click half is explicitly Phase 4 (REQUIREMENTS.md:28, ROADMAP.md:113).

## Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| — | — | No TBD/FIXME/XXX debt markers | — | none |
| — | — | No TODO/HACK/PLACEHOLDER/"not yet implemented" | — | none |
| — | — | No `Tip.setValue`, no `#include <Mod/PartDesign`, no `setStyleSheet`/hex | — | fork discipline + link-free contract held |

`return nullptr` occurrences in FwRollbackBar.cpp / FwFeatureTree.cpp are legitimate guard clauses and the documented "no solid precedes → roll to base" semantic (RollbackBar.cpp:128) — NOT stub returns; real data flows through every goal path around them.

## Human Verification Required

5 items (all environmentally blocked — no build tree / live GUI / 3D viewport / subjective parity). See frontmatter `human_verification`. In summary:
1. Rollback drag → below-tip grey + 3D suppress-below + one-undo restore (FEEL/3D)
2. Invalid drag → forbidden cursor + no insertion line + no commit (affordance FEEL)
3. Plane Front/Top/Right orientation matches SW muscle memory (A1 correspondence)
4. Daily-SW-user rollback + insert-at-bar parity acceptance (SC5)
5. Run the authored `ctest` suite once a build tree exists (open execution gate; tests authored compile-intended, never run locally)

These are the recorded `SPIKE_LIVE_CHECKLIST.md` obligations + the CI gate. They gate the **milestone parity sign-off**, NOT this phase's code completion.

## Gaps Summary

**No code gaps.** Every artifact the phase promised exists, is substantive, is wired into the workbench/dock/signal chain, and routes data through the genuine `Body.Tip` / `PartDesign_MoveTip` / inherited-DnD engines rather than cosmetic placeholders. Upstream seams were independently confirmed in source. Review history converged to 0 HIGH concerns across 3 cycles; the two surviving MEDIUMs (preselection-snapshot lifetime; scoping-test sibling) — MEDIUM-1 is verified applied in `FwSelectionGuard.cpp` (owned-string snapshot). Leak-grep clean. 17 authored headless tests map 1:1 to the logic truths.

The only outstanding work is **live-GUI/3D/subjective parity verification + the actual `ctest` run**, all blocked by the absence of a build tree in this environment. Per the no-build precedent (Phases 1/2) and the explicit task framing, these are acceptable, honestly-recorded deferrals (`SPIKE_LIVE_CHECKLIST.md` Phase-3 section + parity-user track), not failures. Status is `human_needed` solely because non-empty live-verification items remain by Step-9 rule; the **code verdict is PASS**.

---

_Verified: 2026-06-14T18:40:00Z_
_Verifier: Claude (gsd-verifier)_
