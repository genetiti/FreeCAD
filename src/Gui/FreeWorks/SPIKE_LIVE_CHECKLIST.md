<!-- SPDX-License-Identifier: LGPL-2.1-or-later -->

# FreeWorks Native-Ribbon Spike — Live Verification Checklist (Phase 02, D-03)

This checklist covers the parts of the **native-vs-SARibbon spike gate** (Plan 02-01
Task 3) that **cannot be automated headlessly**: the on-screen *look* and the live
*context-switch restore* of a throwaway native `FwRibbon` running against a real
PartDesign Body + sketch on a built GUI binary.

The native-vs-SARibbon **verdict is already recorded** as **`native committed`** in
`.planning/phases/02-commandmanager-ribbon/SPIKE.md`, on a **documentation-approval**
basis (no build tree / GUI / CI in the authoring environment; user-authorized per the
Phase 1 precedent `[01-03]`/`[01-04]`). This checklist is the **open obligation** that
confirms that verdict on real hardware. If any **FAIL** is recorded here, the verdict
flips and the SARibbon vendor-vet gate (bottom of `SPIKE.md`) triggers.

> Naming (D-03): the only reference-CAD identifier permitted in *source* is the
> navigation-style type string established in Phase 1. This is a doc artifact; no new
> source identifier carries the reference-CAD name.

## What is already proven (do NOT re-do these manually)

| Invariant | Where it is proven |
|-----------|--------------------|
| `FwRibbon` is a `QTabWidget`; `addTabFromCommandIds` builds N tabs | `tests/src/Gui/FwRibbonWidget.cpp` (Qt offscreen `QApplication`) |
| Curated IDs resolve after module-GUI bootstrap (`PartDesign_Pad`, `PartDesign_Pocket`, `Std_Measure`) | `tests/src/Gui/FwRibbon.cpp` (`Gui_tests_run`) |
| `*_Comp*` group id resolves (split-button raw material) | `tests/src/Gui/FwRibbon.cpp` (`groupCommandResolves`) |
| D-08 omit-missing: bogus id → nullptr, no crash | `tests/src/Gui/FwRibbon.cpp` (`bogusIdResolvesToNull`) |
| 32px `ToolButtonTextUnderIcon`, palette-only color | `src/Gui/FreeWorks/FwRibbon.cpp:65-66` (code review) |

## Build a throwaway spike harness (not the production mount — Plan 02-03 owns that)

1. Build FreeCAD GUI with the FreeWorks submodule linked (any one OS suffices for the
   spike; tri-OS is covered separately by the CI build matrix).

   ```bash
   cmake --preset conda-linux-release
   cmake --build --preset conda-linux-release
   ./build/conda-linux-release/bin/FreeCAD
   ```

2. In a throwaway harness (a scratch branch / local patch — **do not commit** as the
   production mount), construct an `FwRibbon` and wire 3 tabs to real command IDs, then
   mount it in the top toolbar area for the spike only:

   ```cpp
   auto* ribbon = new FreeWorksGui::FwRibbon();
   ribbon->addTabFromCommandIds("Features",
       {"PartDesign_Pad", "PartDesign_Pocket", "PartDesign_CompPrimitiveSubtractive"});
   ribbon->addTabFromCommandIds("Sketch",
       {"Sketcher_NewSketch", "Sketcher_CreateLine", "Sketcher_CreateCircle"});
   ribbon->addTabFromCommandIds("Evaluate",
       {"Std_Measure", "Part_CheckGeometry", "Std_MassProperties"});
   // Mount ribbon in the top toolbar area (spike-only).
   ```

3. Wire a throwaway context-switch subscription (productionized in Plan 02-04):

   ```cpp
   // On the active Gui::Document:
   //   signalInEdit    -> ribbon->setCurrentIndex(<Sketch tab index>)
   //   signalResetEdit -> ribbon->setCurrentIndex(<saved prior index>)
   ```

## Manual checks — record PASS/FAIL with observed values

Run against a live PartDesign Body with at least one sketch.

| # | D-03 item | What to confirm | Observed | PASS/FAIL |
|---|-----------|-----------------|----------|-----------|
| 1 | ≥3 native tabs | Features/Sketch/Evaluate tabs visible, tabbed | confirmed (UAT Test 1) | PASS |
| 2 | large labeled buttons | 32px icon over wrapped label, visibly large | confirmed (UAT Test 1) | PASS |
| 3 | primary fires real cmd | click Pad → PartDesign Pad task opens | confirmed (UAT Test 2) | PASS |
| 4 | working flyout | `*_Comp*` split-button: primary fires, dropdown lists alternatives | confirmed (UAT Test 2) | PASS |
| 5 | **live context restore** | **enter sketch → Sketch tab → exit → prior tab restored** | **after == before — confirmed (UAT Test 4)** | PASS |
| 6 | reads SW-like | bigger than stock toolbar; tabbed; convincing at a glance | confirmed (UAT Test 1) | PASS |

**Item 5 is the headline gate:** mark PASS only if the observed **after** index equals the
observed **before** index (the ribbon restored the previously-active tab after the sketch
was exited). A "the Sketch tab is reachable" note is **NOT** sufficient.

## Outcome

- **All items PASS** → the recorded `native committed` verdict is confirmed on hardware;
  close this obligation by recording the sign-off below and noting it in
  `02-01-SUMMARY.md`.
- **Any item FAIL** (especially item 5) → the verdict flips: edit `SPIKE.md` to
  `SARibbon fallback triggered`, run the vendor-vet gate (MIT v2.8.0 submodule at
  `src/3rdParty/SARibbon`, clear leak/provenance grep, no Pixi dep), and re-scope Plans
  02-02/02-03/02-04 to the SARibbon API before continuing.

## Sign-off

- Verified by: user (via `/gsd-verify-work 2` UAT)  ·  Date: 2026-06-14
- All 6 items PASS. Item 5 (live context restore): user confirmed enter sketch → Sketch
  tab auto-activates → exit → previously-active tab restored (after == before), UAT Test 4.
- Result: **☑ native confirmed** — the `native committed` verdict in `SPIKE.md` is
  confirmed on a live build; SARibbon fallback NOT triggered. Open obligation CLOSED.

---

## Phase 3 — FeatureManager Design Tree (live obligations)

These are the **live-only / FEEL** residue of Phase 3 (the FeatureManager tree, the
rollback bar, and drag-reorder). The **logic halves are already green-by-construction**
in the headless tests cross-referenced below; what remains here is what a headless
target cannot judge: the on-screen FEEL, the 3D viewport, the live pointer/drag loop,
and a daily reference-CAD user's parity verdict. Each item is **OPEN / DEFERRED** —
this authoring environment has **no build tree, no live GUI, and no reference-CAD
install** (the Phase 1/2 verification-deferral precedent, `[02-01]`). **Nothing below is
fabricated as observed**; each must be signed off before the milestone parity gate.

> Naming (D-03, reviewer concern 1): this file is scanned by
> `tools/fw-string-leak-grep.sh`, so it carries **no bare trademark token**. The
> reference product is "reference CAD" / the "SW" abbreviation throughout, exactly as
> the Phase-2 section above; no new trademark identifier is introduced here.

### What the headless tests already prove (do NOT re-do manually)

| Logic invariant | Where it is proven |
|-----------------|--------------------|
| Bar position → preceding-solid snap is link-free (type-name string, no C++ Body helper) | `tests/src/Gui/FwFeatureTree.cpp` (`rollbackBarResolverSnapsToPrecedingSolidLinkFree`) |
| Firing the Tip move sets `Tip.isTouched()` + `mustExecute()==1`; one undo restores (no double-wrap) | `tests/src/Gui/FwFeatureTree.cpp` (`rollbackFireSetsTipAndOneUndoRestores`) |
| The fire restores BOTH the selection AND the preselection (`getCompleteSelection()` + `getPreselection()` round-trip) | `tests/src/Gui/FwFeatureTree.cpp` (`selectionGuardRoundTripsSelectionAndPreselection`) |
| Reversibility: "Roll to End" restores the last solid feature | `tests/src/Gui/FwFeatureTree.cpp` (`rollbackReversibleRollToEndRestoresLastSolid`) |
| Insert-at-bar Tip policy (solid → tip; non-solid → Tip unchanged) | `tests/src/Gui/FwFeatureTree.cpp` (`insertAtBarSolidBecomesTipNonSolidLeavesTip`) |
| Band paints at the tip boundary; grab zone reports `Qt::SizeVerCursor`; below-tip row greyed role; Roll labels | `tests/src/Gui/FwFeatureTreeWidget.cpp` (`test_RollbackBandRendersGreysBelowTipAndRollActions`) |
| Invalid drag-reorder BLOCKED (no transaction); valid reorder commits in one | `tests/src/Gui/FwFeatureTree.cpp` (`blockedReorderOpensNoTransactionValidReorderOpensOne`) |

### Manual checks — record PASS/FAIL with observed values

Run against a live reference-CAD-style FreeWorks build with a real PartDesign Body that
has at least two solid features and a sketch.

- [ ] **(1) 3D suppress-below FEEL (TREE-02 / SC5).** Drag the rollback bar UP the tree.
  *Expect:* the features below the bar grey out in the tree AND the 3D view rolls back to
  show only the geometry up to the new tip (the existing reference-CAD-style tip display
  drives the 3D — this phase adds no 3D code). *Why deferred:* needs a live GUI + 3D
  viewport. *Logic half green:* the Tip move / `mustExecute()` / greying-role tests above.

- [ ] **(2) Reversibility FEEL (TREE-02, Pitfall 4).** Drag the bar back DOWN, then use
  "Roll to End". *Expect:* the model restores forward feature-by-feature; "Roll to End"
  fully restores to the last solid. *Why deferred:* live drag loop. *Logic half green:*
  `rollbackReversibleRollToEndRestoresLastSolid`.

- [ ] **(3) Fire Tip with NO module link + selection AND preselection restored on
  hardware (A3 + reviewer HIGH-B).** With a feature selected and a 3D preselect (hover)
  live, drag the bar to fire the tip move. *Expect:* the tip actually moves on a live
  document through the command-ID-under-guard path with no PartDesign link, AND after the
  fire BOTH the prior selection AND the prior preselect are exactly as before (no spurious
  selection, no clobbered preselect). *Why deferred:* needs live `Gui::Selection` +
  preselect from the 3D view. *Logic half green:*
  `selectionGuardRoundTripsSelectionAndPreselection`.

- [ ] **(4) Insertion-line / forbidden-cursor FEEL (TREE-04).** Drag a feature to a valid
  drop, then to an invalid one (before its parent). *Expect:* the valid drop shows the 2px
  insertion line; the invalid drop shows the forbidden cursor and refuses (no drop, no
  transaction). *Why deferred:* live pointer + drag event loop. *Logic half green:* the
  drag-gate BLOCK test above (the gate logic is GTest-asserted in 03-01/03-02).

- [ ] **(5) SC5 — daily reference-CAD-user acceptance.** A daily reference-CAD user
  exercises rollback suppress-below + insert-at-bar end to end. *Expect:* it FEELS like the
  reference CAD — muscle-memory rollback, no relearning (PITFALLS Pitfall 7, the
  parity-user track). *Why deferred:* subjective parity judgement on a live build.

- [ ] **(6) A1 plane correspondence (carried from `[03-02]`).** Confirm the plane-name
  remap orientation — Front = XY, Top = XZ, Right = YZ — matches the reference CAD against
  a live reference-CAD install / its pinned Help, in the parity-user check. *Expect:* the
  three remapped plane names map to the orientations a reference-CAD user expects. *Why
  deferred:* needs a live reference-CAD install for the orientation comparison.

### Outcome

- **All items PASS** → the Phase-3 TREE-02 / TREE-04 / SC5 / A1 obligations are confirmed
  on hardware; close by recording the sign-off below and noting it in `03-03-SUMMARY.md`.
- **Any item FAIL** → file the regression against the cross-referenced logic test (if the
  logic is wrong) or against the FEEL/parity track (if only the feel is off), and re-open
  the relevant plan before the milestone parity gate.

### Sign-off (Phase 3)

- Verified by: _pending_  ·  Date: _pending_
- Result: **☐ pending** — open obligation; not yet run on a live build (no build tree /
  GUI / reference-CAD install in the authoring environment).

---

## Phase 4 — PropertyManager live FEEL obligations

These are the **live-only / FEEL** residue of Phase 4 (the left-hosted PropertyManager: the
re-hosted `Gui::Control`/`TaskView` in the left slot, the green-check/red-cross header, the
pink active reference box, SW key/focus semantics, and the sketch→feature loop). The
**logic halves are already green-by-construction** in the Wave 0 headless tests
cross-referenced below; what remains here is what a headless target cannot judge: the
on-screen FEEL, the live edit-time workbench-switch behavior, and a daily reference-CAD
user's parity verdict. Each item is **OPEN / DEFERRED** — this authoring environment has
**no build tree, no live GUI, and no reference-CAD install** (the Phase 1/2/3
verification-deferral precedent). **Nothing below is fabricated as observed.**

> Naming (D-03): this file is scanned by `tools/fw-string-leak-grep.sh`, so it carries **no
> bare trademark token**. The reference product is "reference CAD" / the "SW" abbreviation
> throughout, exactly as the Phase-2/3 sections above.

### What the Wave 0 tests already prove (do NOT re-do manually)

| Logic invariant | Where it is proven |
|-----------------|--------------------|
| FLOW-01 sketch identification is link-free (type-name string `"SketcherGui::ViewProviderSketch"`, no Sketcher include) and never auto-launches a feature | `tests/src/Gui/FwPropertyManager.cpp` (`flowDecisionSelectsOnlyForTheSketchContractLiteral`, `flowDecisionNeverAutoLaunchesAFeatureCommand`) |
| Header green-check→`Control().accept()`, red-cross→`Control().reject()` — distinct slots, no new commit pipeline | `tests/src/Gui/FwPropertyManager.cpp` (`headerMapsCheckToAcceptAndCrossToReject`, `controlSingletonExposesAcceptRejectTargets`) |
| The panel host is a `TaskView` (non-modal dock), never a `QDialog::exec()` path | `tests/src/Gui/FwPropertyManager.cpp` (`panelHostIsTaskViewNotAModalDialog`) |
| Pink active-box state machine: exactly one active, revert on deactivate; the active tone READ off the box widget's local `QPalette::Midlight` role (no hex, never blue) | `tests/src/Gui/FwReferenceBoxStyler.cpp` (`exactlyOneBoxActiveAtATime`, `activeToneIsReadFromTheLocalPaletteRole`, `activeToneIsNeverTheReservedBlue`) |
| A real MainWindow registers the Tasks `TaskView` (objectName `"Tasks"`) resolvable offscreen — the baseline the 04-02 left-placement test extends | `tests/src/Gui/FwPropertyManagerWidget.cpp` (`test_bootstrap_taskView_resolvable`) |

### Manual checks — record PASS/FAIL with observed values

Run against a live reference-CAD-style FreeWorks build with a real PartDesign Body that has
at least one sketch and a linear-pattern (2-direction) feature.

- [ ] **(1) Slide-in reveal FEEL (PROP-01 / D-08).** Start a feature/sketch command or
  double-click a tree feature. *Expect:* the PropertyManager appears left-docked (a short
  width/visibility reveal, or instant — D-08 permits instant). *Why deferred:* live GUI.
  *Logic half green:* the non-modal TaskView-host + resolvable Tasks dock tests.

- [ ] **(2) Pink active-box pick FEEL (PROP-02 / D-05/D-06), against the REAL
  `TaskPatternParameters` panel.** Edit a linear-pattern feature (two direction reference
  boxes). Click into one box. *Expect:* exactly THAT box turns pink (the
  `QPalette::Midlight` active tone), picking geometry fills it + highlights + auto-expands,
  clicking the second box moves the pink to it (the first reverts), and the pink is never
  the reserved blue. **Confirm the CORRECT real box colors** (the focus-inference-first
  default must color the armed `activeDirectionWidget`, not the wrong one — A3 / R6-MAJOR2;
  if it colors the wrong box, the gated `// SW-FORK HOOK` accessor on
  `TaskPatternParameters.{h,cpp}` is promoted into 04-03). *Why deferred:* live focus +
  3D pick. *Logic half green:* the pink state-machine + read-the-role tests.

- [ ] **(3) SW Enter / Esc / Tab FEEL (D-10).** With the panel open: Enter → accept
  (green-check), Esc → reject (red-cross), Tab → advance to the next field / reference box.
  *Expect:* SW key muscle-memory holds; Tab does not get stuck or leak focus to the 3D view.
  *Why deferred:* live focus chain. *Logic half green:* the header→accept/reject mapping.

- [ ] **(4) Sketch→feature loop FEEL (FLOW-01 / D-11) — BOTH halves surviving the edit-time
  WB switch (R6-MAJOR1).** Exit a sketch. *Expect:* (a) the just-finished sketch stays
  PERSISTENTLY selected as the profile, AND (b) the ribbon lands the Features tab ready —
  with NO feature auto-launched (the user clicks Extrude/Revolve). **Both halves must
  survive** the Sketcher-edit workbench round-trip (the sketch edit transiently activates
  `SketcherWorkbench`). *Why deferred:* live edit lifecycle. *Logic half green:* the FLOW-01
  decision-core (select-and-handoff, never auto-launch).

- [ ] **(5) A2 — dock stays left through the WB-switch round-trip.** Double-click a
  PartDesign feature in the tree (PartDesign `setEdit` transiently activates
  `PartDesignWorkbench` via `assureWorkbench`). *Expect:* the `"Tasks"` dock STAYS LEFT
  through the round-trip — it does NOT jump back right mid-edit (A2: no synchronous
  right-move on `deactivated()`). *Why deferred:* live WB switch.

- [ ] **(6) A4 — overlays SURVIVE the edit-time WB switch (R3-ROOT/R4-BLOCKER/R6-MAJOR1).**
  On the same double-click edit (and on a sketch edit): *Expect:* the FreeWorks chrome —
  the header, the key filter, the pink styler — AND the FLOW-01 reset handler BOTH halves
  still FUNCTION through the edit-time WB switch. Concretely: a tree double-click of a
  PartDesign feature opens the panel LEFT **with** the chrome (header/keys/pink), and a
  sketch exit still auto-selects the sketch AND lands the Features tab. *Expect FAIL signal:*
  the panel opens chrome-less, or the sketch-exit selection / Features-tab half does not
  fire (the chrome / handler was torn down mid-edit). *Why deferred:* live edit lifecycle.

- [ ] **(7) Synchronous-within-one-turn confirmation (the basis the singleShot cancellation
  relies on).** Confirm at runtime that the `assureWorkbench → deactivated() → activated() →
  setEdit() → signalInEdit` sequence is ONE synchronous event-loop turn, so a
  `QTimer::singleShot(0)` teardown scheduled in `deactivated()` does NOT fire until AFTER
  `signalInEdit` has cancelled it. *Expect:* no teardown fires between `deactivated()` and
  `signalInEdit` within the same turn. *Why deferred:* needs a live event loop to observe
  the ordering. *If FALSE:* the deferred-cancellable teardown (A4) does not hold and the A4
  `overlays-lost` escalation triggers.

- [ ] **(8) Cross-phase ribbon observation (carried from A4 — flag, do NOT patch here).**
  Confirm whether the Phase 2-3 ribbon (`s_ribbonContext`, reset synchronously in
  `unmountRibbon` at `FwLayout.cpp:280-286`) ALSO vanishes during an edit-time WB switch
  (the same synchronous-teardown-on-deactivated pattern). *Expect:* if it vanishes, file a
  follow-up for the ribbon to adopt the same deferred-cancellable teardown — **do not patch
  the shipped Phase 2-3 code in Phase 4.**

- [ ] **(9) Type-name literal live re-validation (Pitfall 2 / carried).** Confirm
  `"SketcherGui::ViewProviderSketch"` still matches the live sketch view-provider type on
  the current `main` (an upstream rename silently breaks FLOW-01).

### Outcome

- **All items PASS** → the Phase-4 PROP-01/PROP-02/TREE-03/FLOW-01 FEEL obligations and the
  A2/A4 survivability observations are confirmed on hardware; close by recording the sign-off
  below and noting it in `04-01-SUMMARY.md` (and the downstream plan summaries).
- **Any item FAIL** → file the regression against the cross-referenced Wave 0 logic test (if
  the logic is wrong) or against the FEEL/parity track (if only the feel is off). A FAIL on
  (6)/(7) triggers the A4 `overlays-lost` escalation; a wrong-box FAIL on (2) promotes the
  A3 gated `// SW-FORK HOOK` accessor; an A1 FAIL flips the verdict to `thin-adopt`.

### Sign-off (Phase 4)

- Verified by: _pending_  ·  Date: _pending_
- Result: **☐ pending** — open obligation; not yet run on a live build (no build tree /
  GUI / reference-CAD install in the authoring environment).
