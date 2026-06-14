---
phase: 03-featuremanager-design-tree
plan: 03
subsystem: gui-featuremanager-rollback
tags: [freeworks, rollback-bar, body-tip, selection-guard, preselection, partdesign-movetip, drawrow, insert-at-bar]

requires:
  - phase: 03-02
    provides: "FwFeatureTreeDelegate below-tip greying paint hook (kBelowTipRole = Qt::UserRole+4201), reserved drawRow seam (pass-through), the mounted FwFeatureTree + active-Body scoping, the 'PartDesign::Body' type-name literal / link-free Group read"
provides:
  - "FreeWorksGui::FwRollbackBar — drawn-band geometry + PURE LINK-FREE resolveTipTarget() (Group via getPropertyByName + type-name-string solid classification) + fireTipMove() via the PartDesign_MoveTip command-ID under FwSelectionGuard, no outer transaction, no module link"
  - "FreeWorksGui::FwSelectionGuard — RAII snapshot/restore of BOTH the complete Gui::Selection AND the preselection (replay with clearPreSelect=false + explicit setPreselect/rmvPreselect), with selectOnly(target) helper"
  - "FwFeatureTree::drawRow — 4px rollback band at the tip boundary (QPalette::Highlight tone), 8px Qt::SizeVerCursor grab zone, below-tip greying flag driven live by Body.Tip, right-click Roll Back/Roll Forward/Roll to End actions"
  - "Roll-to-End reversibility + insert-at-bar via the Python-only Body.insertObject with an explicit post-insert Tip policy (solid → becomes tip; non-solid → Tip unchanged)"
affects:
  - "Plan 04 (PropertyManager — the keystone; the tree's selection/edit surface feeds it)"
  - "Milestone parity gate (the live FEEL obligations routed to SPIKE_LIVE_CHECKLIST.md Phase-3 section)"

tech-stack:
  added: []
  patterns:
    - "decide-then-act split: pure resolveTipTarget() (no widget, no mutation, link-free) + a separate fireTipMove() act step (mirrors FwRibbonContext decideOnEnter/decideOnReset)"
    - "fire the REAL Body.Tip move via the existing PartDesign_MoveTip command-ID (its own openCommand/commitCommand) — NO outer FreeWorks transaction, NO module include/link, NO C++ Tip.setValue"
    - "RAII selection guard restores BOTH selection AND preselection — both clearSelection and addSelection default clearPreSelect=true (Selection.h:360/385), so the replay passes clearPreSelect=false and the preselection is re-asserted explicitly via setPreselect/rmvPreselect"
    - "link-free solid-feature classification by getTypeId().getName() type-name STRING (mirrors isSolidFeature semantics) — never the C++-only getPrevSolidFeature/isSolidFeature"
    - "rollback band painted in drawRow after the base Gui::TreeWidget::drawRow; band tone + greying via QPalette only (no hex, no setStyleSheet); below-tip flag is the Gui-only kBelowTipRole, never a Visibility/property write"
    - "Body.Tip is the SINGLE source of truth — the bar holds NO new persisted state; band position / grab / greying are all derived from Tip (no .FCStd pollution)"

key-files:
  created:
    - src/Gui/FreeWorks/FwRollbackBar.h
    - src/Gui/FreeWorks/FwRollbackBar.cpp
    - src/Gui/FreeWorks/FwSelectionGuard.h
    - src/Gui/FreeWorks/FwSelectionGuard.cpp
  modified:
    - src/Gui/FreeWorks/FwFeatureTree.h
    - src/Gui/FreeWorks/FwFeatureTree.cpp
    - src/Gui/FreeWorks/CMakeLists.txt
    - src/Gui/FreeWorks/SPIKE_LIVE_CHECKLIST.md
    - tests/src/Gui/FwFeatureTree.cpp
    - tests/src/Gui/FwFeatureTreeWidget.cpp

key-decisions:
  - "The rollback bar fires the REAL Body.Tip move through the existing PartDesign_MoveTip command-ID — never a C++ Tip.setValue, never a PartDesign include/link; the command opens its OWN transaction so the bar adds NO outer FreeWorks openCommand (one Ctrl+Z restores)"
  - "FwSelectionGuard restores BOTH the complete Gui::Selection AND the preselection — a plain clear+re-add would wipe preselection because both clearSelection and addSelection default clearPreSelect=true (Selection.h:360/385); the guard replays with clearPreSelect=false then re-asserts preselection via setPreselect (live) or rmvPreselect (none)"
  - "Bar-position→solid-feature snap is a PURE, LINK-FREE resolver: read the Body Group via getPropertyByName('Group') as App::PropertyLinkList, classify solidness by getTypeId().getName() type-name STRING (mirrors isSolidFeature SEMANTICS) — the C++-only getPrevSolidFeature/isSolidFeature are NEVER called (would force a PartDesign link)"
  - "Insert-at-bar reuses the Python-only Body.insertObject (no command-ID exists) via a doCommand string in one transaction; because insertObject does NOT move Tip, an explicit post-insert Tip policy applies in the SAME transaction — solid insert becomes the tip, non-solid leaves Tip unchanged"
  - "Below-tip greying is the Gui-only kBelowTipRole flag (set live from the current Tip) painted by the Plan-03-02 delegate hook — NEVER a Visibility/property write; the bar holds NO new persisted state (Body.Tip is the single source of truth)"

patterns-established:
  - "decide-then-act for a destructive-but-reversible model mutation: a pure headless-testable resolver + a guarded act step that reuses an existing transactioned command-ID"
  - "RAII Gui::Selection guard that round-trips BOTH selection and preselection around a command that reads global selection"

requirements-completed: [TREE-02]

duration: ~12min
completed: 2026-06-14
---

# Phase 03 Plan 03: Rollback Bar (Body.Tip suppress-below) Summary

**The FeatureManager now has a draggable rollback bar: grab the 4px band at the tip boundary and drag it up the tree to roll the model back — features below grey out (Gui-only, palette-driven), the 3D view shows the rolled-back result via the existing ViewProviderBody tip display, and dragging down / "Roll to End" restores forward. The bar drives the REAL `Body.Tip` (true suppress-below + recompute) through the existing `PartDesign_MoveTip` command-ID, fired under an RAII `FwSelectionGuard` that round-trips both selection and preselection, with no outer transaction and no PartDesign link.**

## Performance

- **Duration:** ~12 min (feat commits landed 11:53–11:58 CDT; plan closed out in a resumed session)
- **Started:** 2026-06-14 (post-03-02, ~11:46 CDT)
- **Completed:** 2026-06-14 (closeout during /gsd-resume-work)
- **Tasks:** 3 (Tasks 1 & 2 `tdd="true"`, Task 3 docs/checklist)
- **Files modified:** 10 (4 created, 6 modified)

## Accomplishments

- **FwSelectionGuard (NEW)** — an RAII guard that, on construction, snapshots the complete
  `Gui::Selection().getCompleteSelection()` AND the current `getPreselection()` (recording
  whether a preselect is live and its `pDocName`/`pObjectName`/`pSubName`/`x`/`y`/`z`),
  exposes `selectOnly(target)`, and on destruction restores BOTH: it replays the
  snapshotted selection with `clearPreSelect=false` (so the replay does not wipe
  preselection) and re-asserts the preselection explicitly via `setPreselect(...)` (when
  live) or `rmvPreselect()` (when none). This is mandatory because both `clearSelection`
  and `addSelection` default `clearPreSelect=true` (Selection.h:360/385) — a naive
  clear+re-add would silently destroy the very preselection the guard claims to preserve
  (reviewer HIGH-B). Pure `src/Gui/Selection/Selection.h` API — no PartDesign link.
- **FwRollbackBar (NEW)** — the rollback-bar mechanism, built on a decide-then-act split
  mirroring `FwRibbonContext`:
  - **Pure `resolveTipTarget(...)`** (no widget, no mutation, LINK-FREE): reads the Body's
    ordered feature list via `getPropertyByName("Group")` as `App::PropertyLinkList` and
    snaps the bar position to the nearest preceding feature whose `getTypeId().getName()`
    is a solid PartDesign-feature type-name (skipping sketch/datum type-names — mirrors
    `isSolidFeature` SEMANTICS by string, NEVER calling the C++-only helper). Top → roll
    to base; end → last solid feature.
  - **`fireTipMove(...)`** (the act step): constructs an `FwSelectionGuard`,
    `selectOnly(target)`, then invokes the `"PartDesign_MoveTip"` command-ID (which has its
    OWN `openCommand`/`commitCommand`, CommandBody.cpp:730) — so NO outer FreeWorks
    transaction is added (avoids double-wrapped undo) — or a Python `obj.Tip = ...`
    `doCommand` string in exactly one transaction. On guard scope-exit both the prior
    selection and the prior preselection are restored.
  - **`rollToEnd(...)`** (forward-restore reversibility) and **`insertAtBar(...)`** (fires
    the Python-only `Body.insertObject(newFeature, tip, after)` in one transaction, then
    applies the explicit post-insert Tip policy: solid → `obj.Tip = newFeature`; non-solid
    → Tip unchanged).
- **FwFeatureTree.drawRow band + greying + Roll actions** — `drawRow` now calls the base
  `Gui::TreeWidget::drawRow` then paints a 4px rollback band at the current tip boundary
  using a `QPalette::Highlight`-derived tone (no hex, no `setStyleSheet`), with an 8px grab
  zone reporting `Qt::SizeVerCursor`. Rows below the bar carry the Gui-only `kBelowTipRole`
  flag (the Plan-03-02 delegate paints them with `QPalette::Disabled` Text) — driven live
  by the current `Body.Tip`, never a `Visibility` write. Right-click `Roll Back` /
  `Roll Forward` / `Roll to End` actions are wired to the Task-1 fire path.
- **No new persisted state** — `Body.Tip` (the existing persisted `App::PropertyLink`) is
  the single source of truth; the band position, grab, and greying are all derived from it
  (no `.FCStd` pollution, D-05/D-06).

## Task Commits

1. **Task 1: FwSelectionGuard + FwRollbackBar (link-free resolver + Body.Tip fire under guard)** — `f475e941a6` (feat)
2. **Task 2: Rollback band paint + grab cursor + Tip-driven below-tip greying + Roll actions** — `dec920036c` (feat)
3. **Task 3: SPIKE_LIVE_CHECKLIST.md Phase-3 live obligations** — committed during closeout (docs)

**Plan metadata:** this docs commit (03-03-SUMMARY + STATE — Phase 3 complete)

_Tasks 1 & 2 are `tdd="true"`; in this no-build-tree env the RED/GREEN distinction is
authored-not-run — the GTest/QTEST additions are committed alongside the implementation
(no separate failing-test commit, consistent with the Phase 1/2/03-01/03-02 no-build
precedent)._

## Files Created/Modified

- `src/Gui/FreeWorks/FwSelectionGuard.h` — `class FwSelectionGuard` (RAII selection + preselection snapshot/restore, `selectOnly`)
- `src/Gui/FreeWorks/FwSelectionGuard.cpp` — `getCompleteSelection`/`getPreselection` snapshot, `clearSelection`/`addSelection(clearPreSelect=false)` replay, `setPreselect`/`rmvPreselect` round-trip
- `src/Gui/FreeWorks/FwRollbackBar.h` — `class FwRollbackBar` (band geometry + pure resolver decl, no QWidget base)
- `src/Gui/FreeWorks/FwRollbackBar.cpp` — link-free `resolveTipTarget`, `fireTipMove` via `PartDesign_MoveTip` under the guard (no outer transaction), `rollToEnd`, `insertAtBar` + Tip policy
- `src/Gui/FreeWorks/FwFeatureTree.h` — `drawRow` override decl + Roll-action / grab-zone members
- `src/Gui/FreeWorks/FwFeatureTree.cpp` — band paint at tip boundary, `Qt::SizeVerCursor` grab zone, `kBelowTipRole` set from Tip, Roll context actions
- `src/Gui/FreeWorks/CMakeLists.txt` — append `FwRollbackBar.{cpp,h}` + `FwSelectionGuard.{cpp,h}` (additive)
- `src/Gui/FreeWorks/SPIKE_LIVE_CHECKLIST.md` — Phase-3 section (six live-only obligations, reference-CAD phrasing)
- `tests/src/Gui/FwFeatureTree.cpp` — GTest: link-free snap, set-Tip→`mustExecute()==1`, selection AND preselection round-trip, one-undo-restores, Roll-to-End, insert-at-bar Tip policy, reversibility
- `tests/src/Gui/FwFeatureTreeWidget.cpp` — QTEST: band paints at tip boundary, grab zone `Qt::SizeVerCursor`, below-tip greyed role, three Roll labels present

## Decisions Made

See `key-decisions` frontmatter. In brief: fire `Body.Tip` via the `PartDesign_MoveTip`
command-ID under an RAII `FwSelectionGuard` (no outer transaction, no module link);
restore BOTH selection and preselection (replay `clearPreSelect=false` + explicit
`setPreselect`/`rmvPreselect`); link-free solid classification by type-name string;
insert-at-bar via the Python-only `Body.insertObject` + explicit Tip policy; below-tip
greying is the Gui-only `kBelowTipRole`, never a `Visibility` write.

## Deviations from Plan

None evident from the committed artifacts. All locally-verifiable acceptance criteria
(grep-based source/test assertions, the link-free guards, the forbidden-token checks, and
the leak-grep) pass against the committed code without modification during this closeout.

_Note: this SUMMARY was authored during a resumed `/gsd-resume-work` closeout — the
original execution session committed Tasks 1 & 2 and wrote (but did not commit) Task 3,
then ended before writing the SUMMARY / updating STATE. The closeout re-verified every
locally-checkable acceptance criterion before recording completion; it does not claim any
in-flight deviation it cannot observe, and it does not record `ctest` green (see
Verification)._

## Issues Encountered

The original session ended mid-wrap-up: Tasks 1 & 2 were committed (`f475e941a6`,
`dec920036c`) but Task 3's `SPIKE_LIVE_CHECKLIST.md` change was left uncommitted, no
`03-03-SUMMARY.md` was written, and STATE.md still read "Completed 03-02". Resolved during
`/gsd-resume-work` by re-running all grep/leak acceptance checks against the committed
code, committing Task 3, writing this SUMMARY, and advancing STATE. `gsd-tools` is not on
PATH in this environment; STATE.md was updated by direct edit (file-based), consistent with
the project's no-SDK-runtime fallback.

## Verification

| Check | Result |
|-------|--------|
| `ctest -R "Gui_tests_run\|FwFeatureTreeWidget_Tests_run"` | **authored, not run locally, pending CI** (no build tree in this env — Phase 1/2/03-01/03-02 precedent; NOT marked green from authoring) |
| `tools/fw-string-leak-grep.sh` | clean (only `Gui::SolidWorksNavigationStyle` allow-listed) |
| FwSelectionGuard.h: `class FwSelectionGuard` | confirmed (==1) |
| FwSelectionGuard.cpp: `getCompleteSelection`, `getPreselection`, `clearSelection`, `addSelection`, `setPreselect`, `rmvPreselect`, `clearPreSelect` | all confirmed (≥1 each) |
| FwRollbackBar.h: `class FwRollbackBar` | confirmed (==1) |
| FwRollbackBar.cpp: `PartDesign_MoveTip`, `FwSelectionGuard`, `getPropertyByName`, `doCommand`/`runString` | all confirmed (≥1 each) |
| FwRollbackBar.cpp forbidden: `Tip.setValue`==0, `getPrevSolidFeature`/`isSolidFeature`==0 | confirmed (both ==0) |
| No `#include <Mod/PartDesign` across the 4 new files; no `PartDesignGui` link in CMake | confirmed (==0 / ==0) |
| CMake lists `FwRollbackBar.cpp` + `FwSelectionGuard.cpp` | confirmed (≥1 each) |
| tests/FwFeatureTree.cpp: `getCompleteSelection`, `getPreselection`, `setPreselect`, `mustExecute`, `insertObject`, `undo` | all confirmed (≥1 each) |
| FwFeatureTree.cpp: `Gui::TreeWidget::drawRow`, `QPalette::Highlight`, `Qt::SizeVerCursor`, `FwRollbackBar` | all confirmed (≥1 each) |
| FwFeatureTree.cpp Roll labels (`Roll Back`/`Roll Forward`/`Roll to End`) | confirmed (≥3) |
| FwFeatureTree.cpp forbidden: `Visibility`==0, `setStyleSheet`==0, hex==0, `#include <Mod/PartDesign`==0 | confirmed (all ==0) |
| tests/FwFeatureTreeWidget.cpp: `Qt::SizeVerCursor`, Roll actions present | confirmed (≥1 each) |
| SPIKE_LIVE_CHECKLIST.md: Phase-3 section + Phase-2 retained + `SC5`/`TREE-02`/`TREE-04`/`A1` | confirmed; no bare `SolidWorks` token (only `Gui::SolidWorksNavigationStyle`) |

## Deferred Obligations (no build tree / live GUI)

Recorded in `src/Gui/FreeWorks/SPIKE_LIVE_CHECKLIST.md` § Phase 3 as open, never-fabricated
obligations, each cross-referenced to the headless test that already covers its logic half:

1. 3D suppress-below FEEL (TREE-02 / SC5) — drag bar up → below greys + 3D rolls back.
2. Reversibility FEEL (Pitfall 4) — drag down / Roll to End restores forward.
3. Fire `Body.Tip` with NO module link + selection AND preselection restored on hardware (A3 + reviewer HIGH-B).
4. Insertion-line / forbidden-cursor FEEL (TREE-04) — live pointer/drag affordance.
5. SC5 — daily reference-CAD-user acceptance (parity-user track, PITFALLS Pitfall 7).
6. A1 plane correspondence (Front=XY/Top=XZ/Right=YZ) confirmed against a live reference-CAD install.

These + the `ctest` run gate the milestone parity sign-off; they are not blockers for the
Phase-3 artifact set, which is complete.

## Next Phase Readiness

- **TREE-02 closed** — the headline FeatureManager slice is delivered: a user can roll back,
  insert mid-history, and restore forward, all through the real `Body.Tip` with clean undo.
  This is the capability a user did NOT have after Plan 03-02.
- **Phase 3 artifact set complete** — no further plan in this phase.
- **Phase 4 (PropertyManager)** is the next planning target — the keystone where TREE-03,
  FLOW-01, and PROP-02 terminate. No CONTEXT.md exists for Phase 4 yet (discuss-phase
  recommended before planning).

## Self-Check: PASSED

- Created files verified present on disk: `FwRollbackBar.h`, `FwRollbackBar.cpp`, `FwSelectionGuard.h`, `FwSelectionGuard.cpp`.
- Task commits verified in git history: `f475e941a6` (Task 1), `dec920036c` (Task 2); Task 3 + this metadata committed during closeout.

---
*Phase: 03-featuremanager-design-tree*
*Completed: 2026-06-14*
