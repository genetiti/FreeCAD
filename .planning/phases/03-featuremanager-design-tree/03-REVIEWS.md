---
phase: 3
reviewers: [codex]
reviewed_at: 2026-06-14
cycle: 2
plans_reviewed: [03-01-PLAN.md, 03-02-PLAN.md, 03-03-PLAN.md]
current_high: 2
cycle_history:
  - cycle: 1
    current_high: 7
    source_grounding: 30/30 verified, 0 MISSING
  - cycle: 2
    current_high: 2
    source_grounding: 11/11 new symbols verified, 0 MISSING
    prior_high_resolved: 5/7 (FULLY); 2/7 PARTIAL (kept open)
---

# Cross-AI Plan Review — Phase 3 (FeatureManager Design Tree)

> **CYCLE 2 (convergence) is recorded at the TOP. Cycle 1 follows below for history.**

---

# ═══════════════════════════════════════════════════════════
# CYCLE 2 — Convergence Re-Review (2026-06-14)
# ═══════════════════════════════════════════════════════════

**Reviewer:** Codex CLI (codex-cli 0.139.0, default model), adversarial re-review of the REVISED 03-01/02/03 plans.
**Mandate:** Verify whether EACH of the 7 cycle-1 HIGH concerns is GENUINELY closed in the revised plan text (not wording-only), and surface any remaining or newly-introduced HIGH concerns.
**Method:** Independent source-grounding of every NEW symbol the revision cites, then a Codex adversarial pass with the per-concern verdict mandate. Both passes converged.

## Cycle-2 Source-Grounding Pass (new symbols cited by the revision)

Every NEW existing symbol the revised plans cite was checked against the real source. New-artifact names introduced by the revision (`FwSelectionGuard`, `FwFeatureTree`, `FwRollbackBar`, `FwFeatureTreeDelegate`) were confirmed not-yet-existing and excluded.

| # | Symbol / Claim (NEW this cycle) | Status | Actual location / note |
|---|---|---|---|
| 1 | `QTreeWidgetItem::setHidden(...)` used by base `Gui::TreeWidget` itself | VERIFIED | Tree.cpp:4574 (`item->setHidden(true)`), :6070 (`objitem->setHidden(...)`); also 3458/3598/3986/5794/6040 — the public-item hide API is real and used internally |
| 2 | `Gui::Selection::getCompleteSelection()` → `std::vector<SelObj>` | VERIFIED | Selection.h:605 (exact) |
| 3 | `addSelection(const SelectionObject&, bool clearPreSelect=true)` / `clearSelection(..., bool clearPreSelect=true)` / `getPreselection()` | VERIFIED (nuance) | Selection.h:360 / :385 / :427 — **both clearSelection AND addSelection DEFAULT to `clearPreSelect=true`** (drives residual HIGH-B below) |
| 4 | `setPreselect(...)` / `rmvPreselect(...)` (preselection restore API) | VERIFIED | Selection.h:413 / :423 — the API to restore preselection EXISTS but the plan does not use it (residual HIGH-B) |
| 5 | `App::PropertyLinkList Group;` (GroupExtension) read link-free via `getPropertyByName("Group")` | VERIFIED | GroupExtension.h:142; upstream itself does `freecad_cast<App::PropertyLinkList*>(targetObj->getPropertyByName("Group"))` at Tree.cpp:3266 → `setValue` :3273 (the exact link-free reorder path the plan reuses) |
| 6 | `Body.insertObject(feature, target, after)` Python-only exposure | VERIFIED | Body.pyi:24 (exact); the `@note the method doesn't modify the Tip unlike addObject()` is at Body.pyi:34 (exact) |
| 7 | `CmdPartDesignMoveTip` owns its OWN `openCommand` (no outer wrap needed) | VERIFIED | CommandBody.cpp:730 (exact); getSelection().getObjectsOfType @674; "Only a solid feature can be the tip" guard @719; FCMD_OBJ_SHOW @739; updateActive @744 |
| 8 | `Body::getPrevSolidFeature`/`isSolidFeature` are C++-only, ABSENT from Body.pyi | VERIFIED | Body.cpp:102 / :176 exist; grep of Body.pyi finds NEITHER — referencing them from FreeWorks WOULD force a PartDesign link (confirms concern-7 premise; the link-free resolver is the correct fix) |
| 9 | `Body::mustExecute()` returns 1 on `Tip.isTouched()` | VERIFIED | Body.cpp:94-100 (exact) — the real suppress-below trigger the GTest asserts |
| 10 | Base `TreeWidget::dragMoveEvent` calls `QTreeWidget::dragMoveEvent` + `canDropObjectEx` | VERIFIED | Tree.cpp:2348 (fn), :2359 (`QTreeWidget::dragMoveEvent`), :2450 (`canDropObjectEx`) — "call base first, decorate ignored case" is grounded |
| 11 | `DocumentObjectItem::object()` PUBLIC accessor → `Gui::ViewProviderDocumentObject*`; plane role via `PropertyString Role`/`PlaneRoles` | VERIFIED | Tree.h:495 (public `object()`); Datums.h:47 (`PropertyString Role`), Datums.h:204 (`PlaneRoles[3]={"XY_Plane","XZ_Plane","YZ_Plane"}`) — the delegate's "stock item path" is real, no private internals |

### Cycle-2 Verification coverage
- **Total new symbols checked:** 11 (+ 4 new-artifact names confirmed not-yet-existing, excluded)
- **VERIFIED:** 11 / 11
- **MISSING:** 0
- **AMBIGUOUS:** 0 — but **2 grounded NUANCES drive the surviving HIGHs**: (3) clearSelection AND addSelection both default `clearPreSelect=true`, so the guard's clear+re-add restore wipes preselection unless `setPreselect`/`rmvPreselect` is used; (11)/topology — `DocumentItem` is the top-level `QTreeWidgetItem` per document (Tree.h:348) and objects are CHILDREN (`parent->addChild(item)` Tree.cpp:4562), so a `PartDesign::Body` is NOT a top-level item — the plan's `topLevelItem(i)` scoping + synthetic-top-level QTEST would not exercise the real nested topology.
- **UNCHECKABLE:** 0
- **Independent confirmation:** the orchestrator's grounding pass agreed with Codex on BOTH surviving HIGHs (topology + preselection), strengthening confidence these are real, not reviewer noise.

---

## Codex Cycle-2 Review

**Summary**

The revision closes most of the prior HIGHs, but not all. Concerns 1, 3, 5, 6, and 7 are genuinely addressed with concrete mechanisms and acceptance gates. Concern 2 is only partially resolved because the plan now uses `setHidden`, but its test/gate still proves a synthetic top-level-item case rather than real stock-tree active-Body scoping and Origin-first rendering. Concern 4 is also only partially resolved because `FwSelectionGuard` snapshots preselection but the restore/test criteria only restore/assert complete selection, leaving preselection loss as a real side effect.

### Per-Concern Verdict Table

| # | Cycle-1 Concern | Verdict | Evidence / why-not |
|---|---|---|---|
| 1 | Leak-grep contradiction | **FULLY RESOLVED** | 03-03 mandates `SPIKE_LIVE_CHECKLIST.md` use reference-CAD/SW phrasing and no bare token (03-03-PLAN.md:35), with explicit task instructions + a direct grep check (03-03-PLAN.md:191-215). Real mechanism, not a claim. Grounding: leak-grep DOES scan markdown; fix is correct. |
| 2 | D-03 spike scoping render | **PARTIALLY RESOLVED** | Mechanism correctly changed to `QTreeWidgetItem::setHidden(true)` (03-01-PLAN.md:26, :167, :173); doc-only PASS rejected (03-01-PLAN.md:211, :224). BUT the QTEST uses two manually-added **top-level** `QTreeWidgetItem`s (03-01-PLAN.md:132) and the impl text assumes non-active Body rows are top-level (03-01:26,:167; 03-02:149,:153). Real stock tree nests Bodies UNDER a `DocumentItem` — the gate can pass without proving real-DOM scoping or Origin-first. → **HIGH-A below.** |
| 3 | Fallback path not credible | **FULLY RESOLVED** | Concrete projection tree building own items from `Group`, reusing command paths, explicitly NOT "override more virtuals" (03-01-PLAN.md:97,:99,:221; 03-02-PLAN.md:69). Closes the non-virtual/friended item-factory problem. |
| 4 | MoveTip global `Gui::Selection` side effects | **PARTIALLY RESOLVED** | `FwSelectionGuard` snapshots selection + preselection, selects only target, restores on scope-exit (03-03-PLAN.md:27,:126,:134). BUT restore/test only re-adds selected `SelObj`s and asserts `getCompleteSelection()` (03-03:126,:131,:140,:145); does NOT restore/assert `getPreselection()`. Since clearSelection/addSelection default to clearing preselect, a visible global side effect can remain. → **HIGH-B below.** |
| 5 | Transaction double-wrap | **FULLY RESOLVED** | Command-ID path explicitly adds NO outer FreeWorks transaction (PartDesign_MoveTip owns openCommand@730) (03-03:28,:115,:128,:142); one `doc->undo()` restores prior Tip required (03-03:131,:145). |
| 6 | `insertObject` mischaracterized | **FULLY RESOLVED** | Now Python-only, no command-ID, doesn't move Tip, explicit post-insert Tip policy (03-03:34,:119,:130,:134): same-transaction insert + Tip update for solid inserts, Tip unchanged for non-solid. |
| 7 | No-link resolver leaks PartDesign | **FULLY RESOLVED** | Consistently bans `getPrevSolidFeature`/`isSolidFeature`, uses `Group` + type-name classification, grep-gated to 0 (03-01:27,:124,:140,:168; 03-03:26,:127,:143); no PartDesign include/link gates (03-01:183; 03-03:144). |

### Remaining or New Concerns

- **[HIGH-A] Real active-Body scoping is still not proven against the stock tree topology.** The `setHidden` mechanism is correct in principle, but the evidence gate is too shallow. The QTEST (03-01-PLAN.md:132) uses two **synthetic top-level** items, while the impl/acceptance language scopes "top-level" rows (03-01:167,:173; 03-02:149,:153). In the real `Gui::TreeWidget`, the per-document `DocumentItem` is the top-level `QTreeWidgetItem` and Bodies/features are nested CHILDREN below it (grounding: Tree.h:348 `class DocumentItem: public QTreeWidgetItem`; Tree.cpp:4562 `parent->addChild(item)`). A `topLevelItem(i)` scope that hides "non-active top-level items" would iterate document items, not Body items — the test would pass while never exercising the real nested topology, and Origin-first ordering is unproven. **Required change:** make `scopeToActiveBody()` traverse recursively from `invisibleRootItem()`, keep required ancestors visible, hide non-active Body subtrees + unrelated objects; the item-2 PASS must assert (via a QTEST/live spike on a real populated FreeCAD document with TWO Bodies) that the active Body is visible, the non-active Body subtree is hidden, and the active Body's Origin renders first among visible rows.

- **[HIGH-B] `FwSelectionGuard` snapshots preselection but does not actually restore it.** The plan snapshots preselection via `getPreselection()` (03-03:27,:126,:134) but the restore wording only re-adds selected `SelObj`s and the test only asserts `getCompleteSelection()` (03-03:131,:140,:145). Grounding confirms both `clearSelection(..., clearPreSelect=true)` and `addSelection(..., clearPreSelect=true)` DEFAULT to clearing preselection (Selection.h:360,:385), so the guard's clear+re-add restore wipes the prior preselection — a visible global state leak the guard was introduced to prevent. The restore API exists (`setPreselect` Selection.h:413 / `rmvPreselect` :423) but is unused. **Required change:** explicitly restore (or explicitly rmv) preselection from the saved `SelectionChanges` using `setPreselect(...)`/`rmvPreselect(...)`; ensure the replay path uses `clearPreSelect=false` where appropriate so it does not clobber the restored preselect; add a GTest comparing pre-fire vs post-fire `getPreselection()` in addition to `getCompleteSelection()`. (Severity note: preselection is transient hover state, so the user-visible impact is mild — but the plan explicitly CLAIMS snapshot-and-restore of preselection and does not deliver it, so the concern stays HIGH until the gate matches the claim.)

- **[MEDIUM] The command-ID no-double-wrap undo test could accidentally exercise only the Python path.** "Python/command path" (03-03:131) is ambiguous; acceptance requires `PartDesign_MoveTip` to appear + no outer `openCommand` (03-03:142), but the undo assertion should explicitly run the COMMAND-ID path when the command is available, and separately run the Python fallback if used.

- **[MEDIUM] Insert-at-bar same-transaction behavior is specified but not directly undo-tested.** The plan says insert + Tip policy happen in one transaction (03-03:130,:134) but acceptance only says "the policy holds" (03-03:145). Add an assertion that ONE undo reverses BOTH the `insertObject` placement and the same-transaction Tip change (proves they are genuinely one transaction, not two undo steps).

### Risk Assessment

Overall risk: **MEDIUM** (down from cycle-1 HIGH). The revised plans are much more executable and resolve the original architectural contradictions, but two gates still allow false positives: stock-tree scoping can pass on synthetic top-level items, and selection restoration can still leak preselection.

- **03-01: MEDIUM-HIGH** — the spike gate is the foundation and currently does not prove real stock-DOM scoping (HIGH-A).
- **03-02: MEDIUM** — depends on the 03-01 scoping assumption; the DnD base-first + delegate plans are otherwise credible.
- **03-03: MEDIUM** — rollback/transaction/no-link mechanics are mostly sound, but `FwSelectionGuard` needs explicit preselection restore + stronger transaction tests (HIGH-B).

---

## Cycle-2 Consensus Summary

Single external reviewer (Codex) this cycle, cross-checked against an independent 11/11 source-grounding pass; both converge. **5 of the 7 cycle-1 HIGH concerns are FULLY RESOLVED with real mechanism changes** (leak-grep token ban + grep gate; concrete projection-tree fallback; no-outer-transaction command-ID path; Python-only insertObject with explicit Tip policy; link-free type-name resolver with grep gates) — these are genuine fixes, not wording-only. **2 concerns are only PARTIALLY RESOLVED and remain open:**

1. **(was concern 2) Scoping render topology** — the mechanism is right (`setHidden`), but the test/gate proves a synthetic top-level-item case, not the real nested `DocumentItem`→Body topology, and Origin-first is unproven. The `topLevelItem(i)` assumption is incorrect for the stock tree (grounding-confirmed: Bodies are children of the per-document `DocumentItem`).
2. **(was concern 4) Preselection restore** — `FwSelectionGuard` snapshots preselection but the restore + the acceptance test only handle `getCompleteSelection()`; the clear+re-add path clears preselection by default, so the side-effect the guard exists to prevent persists. The `setPreselect`/`rmvPreselect` API exists but is unused.

Two new MEDIUMs (explicit command-ID-path undo test; insert-at-bar single-undo test) are tightening suggestions, not blockers.

### Agreed Strengths (cycle 2)
- The revision made REAL mechanism changes for 5/7 HIGHs (verified against plan line + source), not cosmetic rewording.
- Source grounding remains materially accurate (11/11 new symbols verified, 0 MISSING); the revision cites real seams at real locations, and notably reuses upstream's OWN link-free `getPropertyByName("Group")` reorder path (Tree.cpp:3266/3273).
- The `setHidden` scoping mechanism and the projection-tree fallback are both grounded and credible engines.

### Agreed Concerns (cycle 2 — both HIGH, both grounded)
1. **Scoping topology gate is too shallow** — must traverse from `invisibleRootItem()` and be QTEST/live-proven on a real two-Body document with Origin-first, not synthetic top-level items.
2. **Preselection is snapshotted but not restored** — use `setPreselect`/`rmvPreselect`, replay with `clearPreSelect=false`, and assert `getPreselection()` in the guard test.

### Divergent Views
None — single reviewer; the orchestrator's source-grounding pass agreed with Codex on every factual claim it could check, and independently confirmed BOTH surviving HIGHs (the nested `DocumentItem` topology and the `clearPreSelect=true` defaults). No contradictions.

---

## Recommended Next Step

Feed this cycle-2 review back into planning:

```
/gsd-plan-phase 3 --reviews
```

**2 HIGH concerns remain unresolved this cycle (down from 7).** Re-plan to close them before executing Phase 3:
- **HIGH-A:** rewrite `scopeToActiveBody()` to recurse from `invisibleRootItem()` (Bodies are nested under the document item, not top-level) and re-base the item-2 spike PASS on a real two-Body-document QTEST/live demo proving active-Body visible / non-active hidden / Origin-first.
- **HIGH-B:** make `FwSelectionGuard` actually restore preselection (`setPreselect`/`rmvPreselect`, `clearPreSelect=false` on replay) and assert `getPreselection()` round-trips in the guard GTest.

The two MEDIUMs (explicit command-ID undo path; insert-at-bar single-undo assertion) are worth folding into the same replan.

---

# ═══════════════════════════════════════════════════════════
# CYCLE 1 — Initial Review (2026-06-14) — HISTORY
# ═══════════════════════════════════════════════════════════

**Reviewer:** Codex CLI (codex-cli 0.139.0, default model), independent adversarial pass.
**Scope:** 03-01/03-02/03-03 PLAN.md + CONTEXT/RESEARCH/UI-SPEC/VALIDATION/PATTERNS context.
**Method:** Adversarial challenge on App/Gui separation, the Body.Tip rollback fire path, the D-03 spike-gate soundness, drag-reorder BLOCK, transaction discipline, leak-grep, additive CMake, and no-build-tree verification honesty — backed by a source-grounding pass that verified every cited existing symbol against the real FreeCAD source.

---

## Source-Grounding Pass (pre-review verification)

Every existing symbol the plans cite was checked against the real source (paths under `/Users/nguyenthuong/Repository/FreeCAD`). Symbols the plans declare under "Artifacts this phase produces" (`FwFeatureTree`, `FwFeatureTreeDelegate`, `FwRollbackBar`, the new test files, `SPIKE.md`) were excluded.

| # | Symbol / Claim | Status | Actual location / note |
|---|---|---|---|
| 1 | `App::PropertyLink Tip;` | VERIFIED | src/Mod/Part/App/BodyBase.h:54 (exact) |
| 2 | `Body::mustExecute()` returns 1 on `Tip.isTouched()` | VERIFIED | src/Mod/PartDesign/App/Body.cpp:94-100 (exact) |
| 3 | `Body::insertObject(feature,target,after)` | VERIFIED | Body.cpp:275-321; decl Body.h:84 |
| 4 | `Body::isSolidFeature` false for datum/sketch | VERIFIED | Body.cpp:176-194 (datum→false 183-186, non-PartDesign→false 193) |
| 5 | `getPrevSolidFeature`/`getNextSolidFeature` | VERIFIED | Body.cpp:102-128 / 130-158; decls Body.h:140,146 |
| 6 | `Tip.setValue` on addObject | VERIFIED | Body.cpp:248 (exact) |
| 7 | `"PartDesign_MoveTip"`/`CmdPartDesignMoveTip`, "Only a solid feature can be the tip" guard, set-tip seq | VERIFIED | CommandBody.cpp ctor 659-660; guard @719 (712-721); openCommand@730 → Tip=…@733/736 → updateActive@744 |
| 8 | `Gui::TreeWidget` + public ctor `(const char*, QWidget*=nullptr)` | VERIFIED | Tree.h:60 / :65 (exact) |
| 9 | `drawRow(...) const override` | VERIFIED | Tree.h:147 (exact) |
| 10 | `eventFilter` | VERIFIED | Tree.h:82 |
| 11 | **KEYSTONE** non-virtual item construction, TreeWidget-friended | VERIFIED | `createNewItem`@424, `populateItem`@370, ctors @351/@492 — all NON-virtual; friends @328-331, @479-481, @576-577. No virtual factory seam. |
| 12 | F2 rename `relabelObjectAction`→Key_F2; transactioned `Label.setValue` | VERIFIED (nuance) | Tree.cpp:607-614 — Key_F2 only non-Mac; **Key_Return on macOS** (`#ifndef Q_OS_MAC` @608). Commit in `setData` @6536-6556 (label.setValue @6550). |
| 13 | `sortDroppedObjects` Group reorder | VERIFIED | Function @3222; reorder body propGroup->setValue @3266-3273 |
| 14 | `canDragObjectToTarget`/`canDropObjectEx` | VERIFIED | ViewProvider.h:450 / :483 |
| 15 | `claimChildren` | VERIFIED | ViewProvider.h:427 |
| 16 | ViewProviderBody DnD override + tip display | VERIFIED | canDragObjectToTarget @593-600; updateData/setTipIcon @307-332; show() tip logic @602-641 |
| 17 | `ViewProviderOriginGroupExtension::constructChildren` Origin-first | VERIFIED | ViewProviderOriginGroupExtension.cpp:57-79 ("Origin must be first" @69) |
| 18 | `ViewProviderSketchBased::claimChildren` sketch nest | VERIFIED | ViewProviderSketchBased.cpp:77-86 |
| 19 | `PlaneRoles` XY/XZ/YZ_Plane; plane labels | VERIFIED | Datums.h:204; Datums.cpp:270-272 (tr("XY-plane") etc.) |
| 20 | Gui::Application signals | VERIFIED | Application.h: signalActiveDocument:130 / signalActivatedObject:142 / signalInEdit:154 |
| 21 | `DockWindowManager::registerDockWindow`; FwLayout seams | VERIFIED | DockWindowManager.h:84; FwLayout ensureDock@95-103, Fw_FeatureManager@124-127, mountRibbon@138-235 |
| 22 | `Gui::TreeWidgetItemDelegate` | VERIFIED | Tree.h:50 (forward decl) |
| 23 | FwRibbon.cpp bootstrap/kSketchVpTypeName/decide-then-act | VERIFIED | SetUpTestSuite@46-51; kSketchVpTypeName="SketcherGui::ViewProviderSketch"@163; logic tests @168-260 |
| 24 | FwRibbonWidget.cpp QTEST_MAIN/ensureRealMainWindow | VERIFIED | ensureRealMainWindow@37; QTEST_MAIN@392 |
| 25 | FwTestGuiBootstrap.h imports PartDesignGui/SketcherGui/MeasureGui/MatGui | VERIFIED | fn@54; `new Gui::Application(false)`@77-78; imports @99-102 |
| 26 | tests CMake Gui_tests_run list / setup_qt_test / offscreen | VERIFIED | tests/src/Gui/CMakeLists.txt list 7-19, setup_qt_test(FwRibbonWidget)@23; tests/CMakeLists.txt setup_qt_test fn@13, QT_QPA_PLATFORM=offscreen@38/45 |
| 27 | FwRibbon.h LGPL block; palette-only/no-setStyleSheet | VERIFIED | FwRibbon.h:1-23; FwRibbon.cpp:111 palette-only comment; setStyleSheet absent in FwRibbon.cpp |
| 28 | **CRITICAL** How MoveTip picks target | VERIFIED — reads `Gui::Selection` | CmdPartDesignMoveTip::activated reads `getSelection().getObjectsOfType(Part::Feature)`, requires exactly ONE feature, resolves Body via `PartDesignGui::getBodyFor`. No dialog. Command-ID firing with target pre-selected WILL work (A3 stands) — but depends on global selection state. |
| 29 | **CRITICAL** `insertObject` callable without link / Python-exposed? | VERIFIED w/ correction | Python-exposed (BodyPyImp.cpp:52, Body.pyi:24) — drivable with NO link. BUT **no command-ID** wraps insertObject. Body.pyi:34 explicitly: insertObject does NOT move Tip (unlike addObject). |
| — | `tools/fw-string-leak-grep.sh` | VERIFIED | Scans `src/Gui/FreeWorks/` for bare "SolidWorks" (code+comments+**markdown**); allow-lists ONLY the exact token `Gui::SolidWorksNavigationStyle`. |

### Verification coverage
- **Total checked:** 29 cited symbols + leak-grep tool = **30 targets**
- **VERIFIED:** 30 / 30
- **MISSING:** 0
- **AMBIGUOUS:** 0 symbols missing; **2 plan-wording flags** — (12) F2 binding is Mac-conditional (`Key_Return` on macOS — already noted in the plans, good); (29) `insertObject` has a Python path but **no command-ID**, so 03-03's "command-ID/Python path" phrasing is partly inaccurate.
- **UNCHECKABLE:** 0
- **Line drift:** negligible. Citations were strikingly accurate. The keystone fact (non-virtual item construction, item 11) and the MoveTip-reads-Selection fact (item 28) both hold and directly informed the review below.

---

## Codex Review

## Summary

The plan set is directionally strong on reuse, but not sound enough to execute unchanged. The biggest risk is that it treats the hardest seams as already proven: active-body scoping through a stock `Gui::TreeWidget`, no-link rollback target resolution, and command-driven `Body.Tip` mutation. Several acceptance gates would let paper evidence pass where the implementation risk is precisely live Qt/tree behavior. There is also a concrete leak-grep contradiction in `src/Gui/FreeWorks/SPIKE_LIVE_CHECKLIST.md`.

## Strengths

- The source grounding is materially good. The plans correctly identify `Gui::TreeWidget`, `Body.Tip`, `PartDesign_MoveTip`, `Body::insertObject`, F2 rename, and stock DnD as the right existing machinery.
- The additive FreeWorks approach is mostly disciplined: new sources under `src/Gui/FreeWorks/`, no planned `Tree.cpp` edits, no planned `PartDesignGui` target link.
- Reusing F2 rename is the right call. `DocumentObjectItem::setData` already transaction-wraps `Label.setValue`.
- The plan correctly rejects cosmetic rollback state and new persisted rollback properties.
- The plan recognizes the non-virtual `TreeWidget` item-construction surface as a keystone risk instead of pretending the subclass surface is broad.

## Concerns

- **[HIGH] Leak-grep contradiction will break CI.** Plan 03-03 writes live checklist prose under `src/Gui/FreeWorks/SPIKE_LIVE_CHECKLIST.md` and says descriptive prose naming the reference product is acceptable (`03-03-PLAN.md:185`). That is false. `tools/fw-string-leak-grep.sh` explicitly scans markdown under `src/Gui/FreeWorks` and fails any bare `SolidWorks` token (`tools/fw-string-leak-grep.sh:33-35`, `:79-84`). Change the checklist instructions to ban the token entirely in that file; use `reference CAD`, `SW`, or another allowed phrase, and add a direct grep check on the checklist text.

- **[HIGH] The D-03 spike can be cleared without proving the thing it exists to prove.** The plan requires active-body scoping to be demonstrated (`03-01-PLAN.md:197`), then allows clearing the gate by documentation approval and deferring active-body scoping render to the live checklist (`03-01-PLAN.md:207`, `:210`). That papers over the exact risk. `drawRow` and delegates cannot remove rows from layout or selection; they can only paint. Actual scoping likely needs item hiding after `DocumentItem::populateItem`, whose construction/population surface is non-virtual (`src/Gui/Tree.h:370`, `:424`, `:492`, `:576-577`). Change: item 2 must be live-demonstrated or marked FAIL/UNKNOWN. Do not record `reuse committed` from research-only evidence.

- **[HIGH] The fallback path is not credible enough.** "Override more `TreeWidget` virtuals" is not a real fallback if the blocker is non-virtual `DocumentItem`/`DocumentObjectItem` creation (`03-01-PLAN.md:207`, `03-02-PLAN.md:69`). The relevant factory/populate hooks are not virtual and are behind friend relationships. Change: define a concrete fallback: either a separate FreeWorks projection tree that reuses command paths but not item construction, or a minimal upstream hook with `// SW-FORK HOOK`. Do not leave it as "heavier subclass."

- **[HIGH] `PartDesign_MoveTip` selection side effects are not handled.** The command reads global selection and requires exactly one selected `Part::Feature` (`src/Mod/PartDesign/Gui/CommandBody.cpp:674-700`). Plan 03-03 says to select the target and invoke the command (`03-03-PLAN.md:25`, `:116`) but does not require save/restore of `Gui::Selection`, preselection, selection stack, or tree selection blocking. It also inherits the command's visibility side effect (`CommandBody.cpp:738-739`). Change: add an RAII selection guard: snapshot complete selection/preselection, block tree selection observers if needed, clear/add one target, run command, restore selection, and verify no spurious selection remains.

- **[HIGH] Transaction wrapping is ambiguous and may double-wrap.** Plan 03-03 says `fireTipMove` uses `PartDesign_MoveTip` "inside one `openCommand`/`commitCommand`" (`03-03-PLAN.md:110`, `:116`), but the command itself calls `openCommand` (`src/Mod/PartDesign/Gui/CommandBody.cpp:730`). If FreeWorks opens an outer command and invokes this command, undo nesting can be wrong. Change: command-ID path must not add an outer transaction; Python fallback must explicitly open/commit exactly one command. Add an undo test that one Ctrl+Z restores the prior Tip.

- **[HIGH] `insertObject` is mischaracterized and its Tip semantics are unresolved.** Plan 03-03 says insert-at-bar is fired via the "same command-ID/Python path" (`03-03-PLAN.md:30`, `:112`, `:116`). There is no command-ID for `insertObject`; it is Python-exposed only (`src/Mod/PartDesign/App/Body.pyi:24`). The stub explicitly says it does not modify Tip (`Body.pyi:34`). Change: state Python-only. Then decide the UX: if insert-at-bar should behave like mid-rollback insertion, perform `body.insertObject(...)` and then set `body.Tip = newFeature` in the same transaction, or explicitly document that inserted features remain below Tip and therefore suppressed.

- **[HIGH] The no-link rollback resolver leaks PartDesign semantics.** Plan 03-03 requires `getPrevSolidFeature` / `isSolidFeature` semantics in FreeWorks (`03-03-PLAN.md:24`, `:101-102`, `:123-125`), but those are C++ `Body` APIs (`src/Mod/PartDesign/App/Body.h:102`, `:140`, `:146`) and are not exposed in `Body.pyi` except `insertObject`. Calling them from FreeWorks would require PartDesign headers/linkage. Change: either implement a tested no-link resolver using only generic type-name rules, or add a PartDesign-side command/Python helper. Do not reference `getPrevSolidFeature` from `src/Gui/FreeWorks`.

- **[MEDIUM] The DnD override may break the inherited valid-drop path.** Base `TreeWidget::dragMoveEvent` already calls `QTreeWidget::dragMoveEvent`, adjusts target info, and rejects invalid drops via `canDropObjectEx` (`src/Gui/Tree.cpp:2348-2458`). `dropEvent` then performs transactioned mutation (`Tree.cpp:2546-2549`, `:3162`). Plan 03-02 adds its own `dragMoveEvent` gate (`03-02-PLAN.md:151`, `:155`). Change: call `Gui::TreeWidget::dragMoveEvent(event)` first and only decorate the already-ignored case with cursor/no-line UI. Do not reimplement target validation using incomplete subclass access to private target info.

- **[MEDIUM] Generic App access is mostly sufficient, but not for every claim.** `Tip` and `Group` can be read via `getPropertyByName`, and type-name body detection is fine for identification (`03-01-PLAN.md:118`, `:126`). The leak points are solid-feature classification, previous/next solid traversal, and any direct `Body::insertObject` C++ call. Tighten the plan to require all of those through Python strings or a PartDesign-owned helper, never C++ calls from FreeWorks.

- **[MEDIUM] Verification language overclaims in a no-build-tree environment.** The plans repeatedly say tests are green while also saying they "ride CI" (`03-01-PLAN.md:250`, `03-02-PLAN.md:167`, `03-03-PLAN.md:127`). Worse, 03-01 allows setting `wave_0_complete: true` when tests are merely "CI-green-by-construction" (`03-01-PLAN.md:265`). Change statuses to `authored, not run locally, pending CI`. Only mark green/wave complete after an actual CI/build-tree run.

- **[MEDIUM] Plane delegate access is under-specified.** Plan 03-02 says the delegate keys on `PlaneRoles` through a generic object accessor (`03-02-PLAN.md:116`), but it does not prove that role name is available from the `QModelIndex`/item data without relying on `DocumentObjectItem` internals. Add a spike/test that obtains the underlying plane object and role string through the actual stock tree item path, not a fabricated model.

- **[LOW] Additive CMake discipline is mostly sound.** The plans use `target_sources(FreeCADGui PRIVATE ...)` and test-list additions, with explicit no-`PartDesignGui` checks (`03-01-PLAN.md:151`, `03-02-PLAN.md:163`, `03-03-PLAN.md:124`). Keep this, but add a broader CI guard for forbidden `Mod/PartDesign` includes in all new FreeWorks files.

- **[LOW] Grep gates are useful but too shallow.** `grep -c Visibility` and `grep -c setStyleSheet` catch common mistakes, but Python command strings could still mutate view properties or create style/color side effects. Keep the greps, but do not treat them as behavioral proof.

## Suggestions

- Make `SPIKE.md` verdict rules stricter: `reuse committed` requires a live or built test proving active-body subtree scoping survives repopulate/recompute. Otherwise record `subclass-fallback committed` or `unresolved`.
- Split rollback mutation paths explicitly: `MoveTipCommandPath` with selection guard and no outer transaction; `PythonTipPath` with one explicit command transaction and robust `Gui::Command::getObjectCmd` object references.
- Rewrite insert-at-bar as Python-only and decide the post-insert Tip policy. For SW-like behavior, set Tip to the inserted solid feature in the same undo command.
- Replace `getPrevSolidFeature` references in FreeWorks with either a no-link type-name resolver that is tested against real bodies or a PartDesign-owned helper command.
- Change all no-build verification wording from "passes/green" to "authored, pending CI," and make `03-VALIDATION.md` frontmatter reflect that state.

## Risk Assessment

Overall risk: **HIGH** until the spike and rollback command path are tightened. The architecture is plausible, but the current gates would allow major unknowns to pass.

- **03-01 risk: HIGH.** The spike gate is not sound because it can approve reuse without demonstrating active-body scoping, and the fallback is vague.
- **03-02 risk: MEDIUM-HIGH.** Mounting/delegate/F2 are reasonable, but scoping and DnD override behavior can easily break inherited tree semantics.
- **03-03 risk: HIGH.** Rollback touches global selection, undo transactions, PartDesign-only solid-feature logic, and insert-at-bar Tip semantics. This plan needs the most correction before implementation.

---

## Consensus Summary

Single external reviewer (Codex) this cycle, cross-checked against an independent source-grounding pass. Both converge on the same picture: **the citations are accurate and the reuse architecture is sound in principle, but several acceptance gates would let unproven, high-risk seams pass as done.** The source-grounding pass independently surfaced two of the same defects Codex flagged (the no-command-ID for `insertObject`; the MoveTip dependence on global `Gui::Selection`), strengthening confidence that these are real, not reviewer noise.

### Agreed Strengths
- Source grounding is materially accurate (30/30 verified, 0 MISSING) — the plans cite real seams at real locations.
- Additive FreeWorks discipline (no `Tree.cpp` edit, no `PartDesignGui` link, `target_sources` only) is the right merge-safe approach.
- F2 rename reuse, rejection of cosmetic rollback state, and explicit recognition of the non-virtual item-construction keystone are all correct calls.

### Agreed Concerns (highest priority — all HIGH)
1. **Leak-grep contradiction** — 03-03 Task 3 writes "SolidWorks" prose into `src/Gui/FreeWorks/SPIKE_LIVE_CHECKLIST.md`, which the leak-grep DOES scan (markdown included). The plan's claim that this prose is "acceptable" is false and will fail CI. (Confirmed by both Codex and source-grounding of the tool.)
2. **D-03 spike is not a sound gate** — clearing `reuse committed` by documentation approval while deferring the active-body *scoping render* (the precise unknown) is circular. `drawRow`/delegate can only paint, not remove rows; scoping likely needs the non-virtual item surface. Item 2 must be live-demonstrated or the verdict marked FAIL/UNKNOWN.
3. **Fallback path is hand-wavy** — "override more TreeWidget virtuals" is not credible when the blocker is non-virtual, friended item construction. A concrete fallback (projection tree, or `// SW-FORK HOOK`) must be defined.
4. **MoveTip selection side-effects** — firing the command mutates global `Gui::Selection` and shows the tip feature; needs an RAII selection snapshot/restore guard.
5. **Transaction double-wrap** — `PartDesign_MoveTip` already opens its own command; an outer FreeWorks `openCommand` would nest undo wrongly. Command-ID path must add no outer transaction; Python path opens exactly one.
6. **`insertObject` mischaracterized** — Python-only (no command-ID), and it does NOT move Tip. The plan's "command-ID/Python path" wording and the post-insert Tip policy must be corrected.
7. **No-link resolver leaks PartDesign** — `getPrevSolidFeature`/`isSolidFeature` are C++-only `Body` APIs not in `Body.pyi`; referencing them from FreeWorks forces a link. A no-link type-name resolver (or a PartDesign-owned helper) is required.

### Divergent Views
None — single reviewer. The source-grounding pass agreed with Codex on every factual claim it could check and added corroborating detail (items 28/29). No contradictions to investigate.

---

## Recommended Next Step

Feed this review back into planning:

```
/gsd-plan-phase 3 --reviews
```

7 HIGH concerns remain unresolved this cycle — re-plan to close them (especially the leak-grep contradiction, the spike-gate soundness, the no-link resolver leak, and the rollback transaction/selection discipline) before executing Phase 3.
