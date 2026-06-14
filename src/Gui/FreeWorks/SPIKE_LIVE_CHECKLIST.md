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
