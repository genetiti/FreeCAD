---
phase: 3
reviewers: [codex]
reviewed_at: 2026-06-14
cycle: 1
plans_reviewed: [03-01-PLAN.md, 03-02-PLAN.md, 03-03-PLAN.md]
current_high: 7
source_grounding: 30/30 verified, 0 MISSING
---

# Cross-AI Plan Review — Phase 3 (FeatureManager Design Tree)

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
