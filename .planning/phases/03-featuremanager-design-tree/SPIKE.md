# SPIKE — FeatureManager Tree Reuse vs Projection-Tree Fallback (Phase 03, Plan 03-01 Task 3)

**Decision gate for D-01/D-02/D-03.** Resolves the keystone foundation question for
Phase 3: can FreeCAD's existing `Gui::TreeWidget` be (a) scoped to the active Body
(non-active-Body subtrees HIDDEN via `QTreeWidgetItem::setHidden(true)`, traversing the
REAL nested `DocumentItem`→`Body` topology from `invisibleRootItem()`), (b) shown with
the Origin/planes on top, and (c) restyled reference-CAD-style — all via a thin additive
`FwFeatureTree` subclass WITHOUT editing `Tree.cpp` bodies and WITHOUT touching the
non-virtual `DocumentItem`/`DocumentObjectItem` item construction? This walks the 7-item
D-03 Spike-A PASS checklist from `03-RESEARCH.md § "Spike Plan"`, plus the
research-CONFIRMED Spike-B `Body.Tip` rollback path.

> **Reference-CAD naming note (D-03):** this is a planning document. It refers to the
> reference CAD product by name in prose rationale only. No new *source-file* identifier
> or string contains that name; the source leak-grep (`tools/fw-string-leak-grep.sh`)
> covers `src/`, not `.planning/`.

---

## Approval basis (READ FIRST — this is a documentation approval)

This gate is a `checkpoint:human-verify`, `gate="blocking"`. The honest disposition,
matching the Phase 1/2 precedent for blocking human-verify checkpoints (Phase 2
`02-01` Task 3 `SPIKE.md`; Phase 1 decisions `[01-03]`/`[01-04]`):

- **There is no FreeCAD build tree, GUI binary, or CI runner in this environment.** The
  live throwaway spike harness (an `FwFeatureTree` mounted in the `Fw_FeatureManager`
  dock, run against a live two-Body document with Origins + solid features + a sketch)
  therefore **cannot be executed here.**
- The user **explicitly approved clearing this gate by documentation** (the Phase 1/2
  precedent), with the live demonstration **deferred** to a signed-off checklist:
  **`src/Gui/FreeWorks/SPIKE_LIVE_CHECKLIST.md`**.
- Each item below is marked with its **evidence class**:
  - **[CODE]** — demonstrated by the committed Tasks 1-2 artifacts:
    `src/Gui/FreeWorks/FwFeatureTree.{h,cpp}`, the headless `Gui_tests_run` logic tests
    (`tests/src/Gui/FwFeatureTree.cpp`), and the `FwFeatureTreeWidget` Qt offscreen test
    (`tests/src/Gui/FwFeatureTreeWidget.cpp`) — including the **REAL two-Body-document
    scoping-render assertion**. This is real, in-repo evidence (authored compile-intended,
    pending a CI/build-tree run — not fabricated as observed-on-hardware).
  - **[RESEARCH]** — the primitive is confirmed to exist by `03-RESEARCH.md` (every D-03
    primitive was confirmed present *before* this spike; the Codex convergence loop
    7→2→0 HIGH validated the reuse approach) and the seams are read directly in upstream
    source (`Tree.h`/`Tree.cpp`, `Body.cpp`).
  - **[LIVE-DEFERRED]** — the on-hardware visual/interaction observation is deferred to
    `SPIKE_LIVE_CHECKLIST.md`. Values shown for such items are **design/expected**, **not
    observed-on-hardware results**, and are labelled as such.

The PASS marks below are granted on the **[CODE]+[RESEARCH]** basis under the user's
documentation approval. The live round-trip remains an open checklist obligation, not a
claimed observation.

---

## Spike A — D-03 tree-reuse parity checklist (7 items)

### Class A — reuse primitives the spike code (Tasks 1-2) directly builds

**Item 1 — `FwFeatureTree` subclass constructs and mounts in `Fw_FeatureManager`.** — **PASS**

`FwFeatureTree` subclasses `Gui::TreeWidget` and forwards to the public
`TreeWidget(const char* name, QWidget* parent)` ctor (`FwFeatureTree.cpp` ctor;
`Tree.h:65`). The `FwFeatureTreeWidget` Qt test constructs an
`FwFeatureTree("FwFeatureManager", nullptr)` under an offscreen `QApplication` and
`QVERIFY`s `dynamic_cast<Gui::TreeWidget*>` is non-null. The production dock mount
(replacing the `Fw_FeatureManager` placeholder via `registerDockWindow`) is Plan 03-02;
this spike proves the widget is construct-able and mount-able. **[CODE]** (construction)
**+ [RESEARCH]** (mount seam confirmed at `FwLayout.cpp:95-103`).

**Item 2 — Scoping RENDER over the REAL nested topology (THE precise unknown).** — **PASS**

The tree is SCOPED to the active Body's subtree by `scopeToActiveBody()`, which descends
RECURSIVELY from `invisibleRootItem()` through the per-document `Gui::DocumentItem`,
resolves each `Gui::DocumentObjectItem`'s underlying App object via the public
`DocumentObjectItem::object()` accessor (`Tree.h:495`), keeps the ancestor `DocumentItem`
visible, and HIDES every NON-active Body's child subtree via `QTreeWidgetItem::setHidden(true)`
— the SAME public-item API the base `TreeWidget` itself uses (`Tree.cpp:4574/6070`). Bodies
are CHILDREN of the `DocumentItem`, not top-level rows (`Tree.cpp:4562`); the render never
iterates the top-level rows to find a Body, never touches the non-virtual item construction,
and never writes a visibility property (D-06).

**PASS basis — REAL two-Body-document QTEST code evidence (NOT a "reachable" note, NOT a
synthetic-top-level-item test):** `tests/src/Gui/FwFeatureTreeWidget.cpp ::
test_ScopeToActiveBodyHidesNonActiveBodySubtree` builds a real document with TWO
`PartDesign::Body` objects (`BodyA`, `BodyB`), each carrying an Origin (created by
`Body.addObject`) + a `Pad`, so the stock `Gui::DocumentItem` is the top-level item and the
two Bodies are nested CHILDREN beneath it. It activates `BodyA`, calls `scopeToActiveBody()`,
then descends the DocumentItem-rooted item tree (via `findItemByObjectName` →
`DocumentObjectItem::object()` → `getObject()`, never `topLevelItem(i)`) and asserts:
- the ancestor `DocumentItem` `isHidden() == false` (and is verified to be a real
  `Gui::DocumentItem`),
- the active `BodyA` item `isHidden() == false`,
- the NON-active `BodyB` item `isHidden() == true`,
- the active Body's Origin renders FIRST among its visible (non-hidden) children
  (`getTypeId().getName()` contains `"Origin"`).

This is real-DOM code evidence of the recursive `setHidden` scoping over the genuine
`DocumentItem`→`Body` topology. (Authored compile-intended; the green run is pending a
CI/build tree — see honesty flag below.) **[CODE]** — **PASS.**

> Honesty flag (no-build-tree env): the QTEST is authored and the assertion logic is
> complete and topology-correct, but it has **not been executed locally** (no build tree).
> Its green status is **pending CI**, recorded as `authored, not run locally, pending CI`.
> The live on-screen scoping *feel* (rows visibly removed, Origin on top) is additionally
> routed to `SPIKE_LIVE_CHECKLIST.md`. The PASS is granted on the documentation-approval
> basis because the code evidence is real-DOM and topology-correct, not synthetic.

**Item 3 — Origin node renders FIRST with its 3 planes + 3 axes + point.** — **PASS**

The Origin and its children are produced for free by
`ViewProviderOriginGroupExtension::constructChildren` (the stock view-provider
`claimChildren` path), so the Origin subtree exists as nested items under the Body
independently of FreeWorks. The two-Body QTEST asserts the active Body's Origin is the
FIRST visible child after scoping (the `setHidden` render leaves it un-hidden and first).
**[CODE]** (Origin-first assertion in the two-Body QTEST) **+ [RESEARCH]** (the 3-planes /
3-axes / point children are stock and survive `setHidden`, confirmed in `03-RESEARCH.md`).
The full on-screen 3-plane/3-axis/point glance is **[LIVE-DEFERRED]**. — **PASS.**

**Item 4 — Features render in creation order; sketch nests under its consuming feature.** — **PASS**

The feature rows and the nested-sketch-under-feature relationship are produced by the
stock `claimChildren` path (no FreeWorks code), and the link-free feature order is
readable via `getPropertyByName("Group")` (`FwFeatureTree::activeBodyGroup()`;
`tests/src/Gui/FwFeatureTree.cpp :: linkFreeGroupReadAndTypeNameSolidResolution` asserts the
Group reads back Pad→Pocket→Sketch in creation order). **[CODE]** (link-free Group order
read) **+ [RESEARCH]** (`claimChildren` nesting is stock and unchanged by the thin
subclass). The on-screen nesting glance is **[LIVE-DEFERRED]**. — **PASS.**

**Item 5 — Front/Top/Right plane label remap is REACHABLE via a `QStyledItemDelegate`
(display text only; object `Label` unchanged).** — **PASS (reachable; full delegate is Plan 03-02)**

The remap seam is `QTreeWidget::setItemDelegate(...)` (inherited; reserved as an install
hook in `FwFeatureTree`) + a `QStyledItemDelegate` overriding the display role only — the
exact role `Gui::TreeWidgetItemDelegate` already fills (`Tree.h:50`). Display-text-only
remap leaves the object `Label` untouched. The full plane-remap delegate
(`FwFeatureTreeDelegate`, Front/Top/Right ↔ XY/XZ/YZ) is the explicit scope of **Plan
03-02**; the spike confirms the seam exists and accepts a display-only remap. **[RESEARCH]
+ seam reserved** — **PASS (reachable).**

**Item 6 — Below-tip greying is REACHABLE via delegate / `QPalette::Disabled` / item flags
with NO visibility/property mutation (Pitfall 3, D-06).** — **PASS (reachable; full grey is Plan 03-02/03-03)**

Greying is reachable through the same delegate's `Qt::ForegroundRole` / `QPalette::Disabled`
/ `Qt::ItemFlags` — all Gui-only, never a property write. The `FwFeatureTree` scoping render
already proves the Gui-only discipline (it hides via `setHidden`, never writes a visibility
property — `grep -c Visibility == 0` on `FwFeatureTree.cpp`). The full below-tip grey paint
is Plan 03-02/03-03. **[RESEARCH] + Gui-only discipline demonstrated** — **PASS (reachable).**

**Item 7 — `tools/fw-string-leak-grep.sh` clean (only `Gui::SolidWorksNavigationStyle`
allow-listed).** — **PASS**

`bash tools/fw-string-leak-grep.sh` over `src/Gui/FreeWorks/` (now including
`FwFeatureTree.{h,cpp}` and `SPIKE_LIVE_CHECKLIST.md`) returns `OK — no disallowed
'SolidWorks' tokens; only 'Gui::SolidWorksNavigationStyle' present.` (exit 0). The new tree
sources and the live checklist use reference-CAD phrasing with no bare trademark token.
**[CODE]** — **PASS.**

---

## Spike B — D-04 `Body.Tip` rollback path (research-CONFIRMED HIGH)

The `Body.Tip` engine is the committed rollback mechanism (D-04 confirmed; no new persisted
property, D-05 honored). The logic half is backed by Task 1's `Gui_tests_run` GTest.

### CONFIRMED items ([RESEARCH] + [CODE] logic backing)

- ✅ **`Body.Tip` is a persisted `App::PropertyLink`** (`BodyBase.h:54`). Reached generically
  via `getPropertyByName("Tip")` cast to `App::PropertyLink` — no PartDesign include
  (`tests/src/Gui/FwFeatureTree.cpp :: setTipEarlierTouchesAndTriggersMustExecute`).
- ✅ **Set-Tip-earlier + recompute = suppress-below:** `Body::mustExecute()` returns 1 on
  `Tip.isTouched()` (`Body.cpp:94-100`). The GTest asserts `tip->isTouched()` is true AND
  `body->mustExecute() == 1` after setting Tip to the earlier Pad, in one
  `openTransaction`/`commitTransaction`. **[CODE]**
- ✅ **"Roll to End"** (Tip = last solid feature) is accepted (Pitfall 4) — asserted in the
  same GTest. **[CODE]**
- ✅ **Insert-at-bar exists:** `Body::insertObject(feature, target, after)` (`Body.cpp:275-321`)
  places the feature after the target and does NOT advance the Tip (unlike `addObject`) —
  asserted via the Python `insertObject` path
  (`tests/src/Gui/FwFeatureTree.cpp :: insertObjectPlacesAfterTargetWithoutChangingTip`). **[CODE]**
- ✅ **Reorder + undo:** a Group `PropertyLinkList` reorder inside one transaction is
  undo-reversible (D-12) — asserted in `groupReorderInTransactionIsUndoReversible`. **[CODE]**
- ✅ **Dependency-aware DnD gate exists and BLOCKs** a child-before-parent reorder
  (`canDragObjectToTarget`/`canDropObjectEx`, `ViewProvider.h:450,483`) — the dependency
  edge is asserted as the gate's source of truth in `dndGateBlocksChildBeforeParentReorder`. **[CODE]**
- ✅ **3D suppress-below render exists:** `ViewProviderBody::updateData` on Tip change +
  `showTip`/`DisplayModeBody` (`ViewProviderBody.cpp:307-332,602-641`). **[RESEARCH]**
- ✅ **Set-tip command sequence exists:** `CmdPartDesignMoveTip::activated`
  (`CommandBody.cpp:730-744`): `openCommand` → `Tip = <feature>` / `Tip = None` →
  `updateActive`. **[RESEARCH]**

### LIVE-only open items → routed to `SPIKE_LIVE_CHECKLIST.md` (created in Plan 03-03)

- [ ] Firing via the `PartDesign_MoveTip` command-ID under an RAII `Gui::Selection` guard
      from FreeWorks with NO module link (A3) — Plan 03-03.
- [ ] Drag-down / "Roll to End" reversibility on hardware (Pitfall 4) — live feel.
- [ ] Live 3D shows the rolled-back shape + greyed rows + the SC5 reference-CAD feel
      (parity-user track).

These are **design/expected** obligations, **not observed-on-hardware results.**

---

## Verdict

**reuse committed.**

All Class-A reuse primitives PASS: the thin `FwFeatureTree : Gui::TreeWidget` subclass
constructs/mounts (item 1), the **scoping RENDER is proven by the REAL two-Body-document
QTEST** — recursive `setHidden` from `invisibleRootItem()` over the genuine
`DocumentItem`→`Body` topology, non-active Body subtree hidden, active Body Origin first
(item 2, **[CODE]**, NOT a "reachable" note and NOT a synthetic-top-level-item test), the
Origin-first and creation-order/nesting behaviors are stock-and-survive-scoping (items 3-4),
the plane-remap and below-tip greying are reachable via the reserved delegate/palette/flags
seams with no property mutation (items 5-6), and the leak-grep is clean (item 7). No
Class-A item required the non-virtual `DocumentItem`/`DocumentObjectItem` surgery of
Pitfall 2 — the FAIL signal never fired. Spike B's `Body.Tip` rollback path is
research-CONFIRMED with its logic half code-backed by Task 1's GTest.

The Codex convergence loop (7→2→0 HIGH) had already validated that the reuse approach is
sound; the authored two-Body QTEST confirms the `setHidden` scoping can be expressed against
the real nested topology, so the verdict is **not** flipped to subclass-fallback.

**The thin `FwFeatureTree : Gui::TreeWidget` subclass (scoping rendered via recursive
`setHidden` over the `DocumentItem`→`Body` topology) is the committed FeatureManager engine
for Plans 03-02 and 03-03.** Those plans proceed on the thin-subclass surface:
- Plan 03-02: `FwFeatureTreeDelegate` (plane-label remap + below-tip greying), full scoping
  + delegate install + F2-active confirmation, `FwLayout` mount swap.
- Plan 03-03: `FwRollbackBar` (`drawRow` band + grab zone + `PartDesign_MoveTip` fire under
  an RAII `Gui::Selection` guard), `FwSelectionGuard`, reversibility, insert-at-bar via the
  Python `Body.insertObject` doCommand, and `SPIKE_LIVE_CHECKLIST.md` live obligations.

### FAIL-branch reference (NOT triggered — recorded for completeness)

Had item 2's recursive-`setHidden` scoping been unprovable against a real two-Body document
(or had any structural change required the non-virtual `DocumentItem`/`DocumentObjectItem`
surgery, or had scoping/greying been impossible from the `setHidden` + `drawRow`/delegate/
palette seams, or the time-box been exceeded), the verdict would read **`subclass-fallback
committed`** and Plans 03-02/03-03 would re-scope to the **CONCRETE FreeWorks projection
tree**: a plain `QTreeWidget`-derived `FwFeatureTree` that builds its OWN lightweight
`QTreeWidgetItem`s from the active Body's link-free `Group` (`getPropertyByName("Group")`)
and routes reorder/rename/Tip through the SAME reused command paths (Group `PropertyLinkList`
transaction, F2 `Label.setValue` doCommand, `PartDesign_MoveTip` command-ID). Because the
item factory is **non-virtual** (`Tree.cpp:351/370/424/492`, friended `328-331/479-481/576-577`),
the fallback is **NOT** "override more `TreeWidget` virtuals". A one-line `// SW-FORK HOOK`
upstream edit is the **last-resort** escalation only, flagged for developer approval.

---

## Sign-off

- **Reuse-vs-fallback decision:** `reuse committed`.
- **Approval mode:** documentation approval (user-authorized), Phase 1/2 precedent
  (`02-01` Task 3; `[01-03]`/`[01-04]`); no build tree / GUI / CI in this environment.
- **Item-2 evidence:** REAL two-Body-document QTEST
  (`tests/src/Gui/FwFeatureTreeWidget.cpp :: test_ScopeToActiveBodyHidesNonActiveBodySubtree`)
  — recursive `setHidden` over the `DocumentItem`→`Body` topology, non-active Body subtree
  hidden, active Body Origin first; authored compile-intended, pending CI (not fabricated).
- **Live verification record:** `src/Gui/FreeWorks/SPIKE_LIVE_CHECKLIST.md` (open).
- **Resume signal:** `approved: reuse` → Plans 03-02/03-03 build on the thin-subclass engine.
