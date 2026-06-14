<!-- SPDX-License-Identifier: LGPL-2.1-or-later -->

# FreeWorks FeatureManager-Tree Spike — Live Verification Checklist (Phase 03, D-03)

This checklist covers the parts of the **tree-reuse spike gate** (Plan 03-01 Task 3) that
**cannot be automated headlessly**: the on-screen *look* and the live *active-Body scoping*,
*plane-label remap*, *below-tip greying*, and the Spike-B `Body.Tip` *rollback feel* of a
throwaway `FwFeatureTree` running against a real two-Body document on a built GUI binary.

The reuse-vs-fallback **verdict is already recorded** as **`reuse committed`** in
`.planning/phases/03-featuremanager-design-tree/SPIKE.md`, on a **documentation-approval**
basis (no build tree / GUI / CI in the authoring environment; user-authorized per the
Phase 1/2 precedent). This checklist is the **open obligation** that confirms that verdict
on real hardware. If any **FAIL** is recorded here, the verdict flips and Plans 03-02/03-03
re-scope to the projection-tree fallback (bottom of `SPIKE.md`).

> Naming (D-03): the only reference-CAD identifier permitted in *source* is the
> navigation-style type string established in Phase 1. This is a source-tree artifact
> scanned by `tools/fw-string-leak-grep.sh`; it describes reference-CAD behavior in prose
> and carries no bare reference-CAD trademark token.

## What is already proven (do NOT re-do these manually)

| Invariant | Where it is proven |
|-----------|--------------------|
| `FwFeatureTree` is-a `Gui::TreeWidget`; constructs headless; inherited F2/Return rename action present | `tests/src/Gui/FwFeatureTreeWidget.cpp` (Qt offscreen `QApplication`) |
| Active Body identified by the `"PartDesign::Body"` type-name string only (no module link) | `tests/src/Gui/FwFeatureTree.cpp` (`activeBodyIdentifiedByTypeNameLiteralOnly`) |
| REAL two-Body scoping render: non-active Body subtree hidden, active Body Origin first, via recursive `setHidden` from `invisibleRootItem()` over the `DocumentItem`→`Body` topology | `tests/src/Gui/FwFeatureTreeWidget.cpp` (`test_ScopeToActiveBodyHidesNonActiveBodySubtree`) |
| Link-free `Group` read + type-name solid resolution (no C++-only Body helper) | `tests/src/Gui/FwFeatureTree.cpp` (`linkFreeGroupReadAndTypeNameSolidResolution`) |
| Set-Tip-earlier → `Tip.isTouched()` + `mustExecute()==1`; "Roll to End" accepted | `tests/src/Gui/FwFeatureTree.cpp` (`setTipEarlierTouchesAndTriggersMustExecute`) |
| `insertObject` places after target without advancing Tip | `tests/src/Gui/FwFeatureTree.cpp` (`insertObjectPlacesAfterTargetWithoutChangingTip`) |
| Group reorder in one transaction is undo-reversible | `tests/src/Gui/FwFeatureTree.cpp` (`groupReorderInTransactionIsUndoReversible`) |
| Dependency-aware DnD gate BLOCKs child-before-parent | `tests/src/Gui/FwFeatureTree.cpp` (`dndGateBlocksChildBeforeParentReorder`) |
| Gui-only discipline: scoping hides via `setHidden`, never a visibility property write | `src/Gui/FreeWorks/FwFeatureTree.cpp` (code review; `grep -c Visibility == 0`) |

## Build a throwaway spike harness (not the production mount — Plan 03-02 owns that)

1. Build FreeCAD GUI with the FreeWorks sources linked (any one OS suffices for the spike;
   tri-OS is covered separately by the CI build matrix).

   ```bash
   cmake --preset conda-linux-release
   cmake --build --preset conda-linux-release
   ./build/conda-linux-release/bin/FreeCAD
   ```

2. In a throwaway harness (a scratch branch / local patch — **do not commit** as the
   production mount), construct an `FwFeatureTree`, point it at a live document containing
   TWO Bodies, and mount it in the `Fw_FeatureManager` dock for the spike only:

   ```cpp
   auto* tree = new FreeWorksGui::FwFeatureTree("FwFeatureManager");
   tree->setDocument(Gui::Application::Instance->activeDocument());
   // Activate one Body in the 3D view / tree, then:
   tree->setActiveBody(/* the active PartDesign::Body */);
   tree->scopeToActiveBody();
   // Dock the tree in the Fw_FeatureManager area (spike-only).
   ```

3. Build a live two-Body document: two Bodies, each with an Origin (3 planes + 3 axes +
   point), a couple of solid features, and a sketch nested under its consuming feature.

## Manual checks — record PASS/FAIL with observed values

Run against a live document with TWO active-able Bodies.

| # | D-03 item | What to confirm | Observed | PASS/FAIL |
|---|-----------|-----------------|----------|-----------|
| 1 | construct + mount | `FwFeatureTree` mounts in the `Fw_FeatureManager` dock, no crash | _to record_ | _pending build_ |
| 2 | **live active-Body scoping** | **activate one Body → only that Body's subtree visible, the other Body's rows removed; switch active Body → scope follows** | _to record_ | _pending build_ |
| 3 | Origin on top | the active Body's Origin renders first with its 3 planes + 3 axes + point | _to record_ | _pending build_ |
| 4 | creation order + nesting | features in creation order; profile sketch nested under its consuming feature | _to record_ | _pending build_ |
| 5 | plane-label remap | a display-only delegate remaps the three origin planes to the reference-CAD names; object `Label` unchanged | _to record_ | _pending build_ |
| 6 | below-tip greying | rows below the rollback bar render greyed via palette/flags; no `.FCStd`/visibility mutation | _to record_ | _pending build_ |
| 7 | reads reference-CAD-like | the scoped, Origin-on-top tree reads convincingly at a glance | _to record_ | _pending build_ |

**Item 2 is the headline gate:** mark PASS only if, after activating one Body, the *other*
Body's subtree is **visibly removed** (hidden) and the active Body's Origin is on top — and
the scope **follows** when the active Body changes. A "the active Body subtree is reachable"
note is **NOT** sufficient; the headless `setHidden` assertion in
`test_ScopeToActiveBodyHidesNonActiveBodySubtree` is the code companion to this live check.

## Spike-B `Body.Tip` rollback — live obligations (productionized in Plan 03-03)

| # | Item | What to confirm | Observed | PASS/FAIL |
|---|------|-----------------|----------|-----------|
| B1 | fire via command-ID, no link | `PartDesign_MoveTip` fired under an RAII `Gui::Selection` guard from FreeWorks (target feature selected) sets `Body.Tip` with NO module link | _to record_ | _pending build_ |
| B2 | reversibility | dragging the bar down / "Roll to End" restores the forward Tip (Pitfall 4) | _to record_ | _pending build_ |
| B3 | live 3D suppress-below | the 3D view shows the rolled-back shape; below-tip rows greyed; feels like the reference CAD (SC5) | _to record_ | _pending build_ |

## Outcome

- **All items PASS** → the recorded `reuse committed` verdict is confirmed on hardware;
  close this obligation by recording the sign-off below and noting it in
  `03-01-SUMMARY.md`.
- **Any item FAIL** (especially item 2) → the verdict flips: edit `SPIKE.md` to
  `subclass-fallback committed`, re-scope Plans 03-02/03-03 to the FreeWorks projection-tree
  fallback (own `QTreeWidgetItem`s from the link-free `Group`, reused command paths — NOT
  "override more virtuals"; `// SW-FORK HOOK` last-resort only), and adjust those plans'
  actions before continuing.

## Sign-off

- Verified by: _pending a build tree / live GUI_  ·  Date: _pending_
- Result: **☐ reuse confirmed on hardware** — to be checked when the live spike harness runs.
  Until then the `reuse committed` verdict in `SPIKE.md` stands on the documentation-approval
  basis (REAL two-Body QTEST code evidence for item 2), with this live demonstration as the
  open obligation.
