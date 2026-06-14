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
