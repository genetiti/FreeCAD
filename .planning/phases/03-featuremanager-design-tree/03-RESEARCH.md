# Phase 3: FeatureManager Design Tree - Research

**Researched:** 2026-06-14
**Domain:** FreeCAD Gui-layer tree widget reuse (Qt 6.8 / C++20), PartDesign Body.Tip rollback engine, ViewProvider drag-drop dependency gates
**Confidence:** HIGH (all keystone claims confirmed by direct source read at the canonical file:line seams; SW parity confirmed against version-pinned SOLIDWORKS Help 2013-2025)

## Summary

The single most important finding: **FreeCAD's stock tree already produces the exact SolidWorks structure this phase needs** — Origin-on-top, features in creation order, sketches nested under their consuming feature — entirely through existing ViewProvider `claimChildren`/`extensionClaimChildren` machinery, with **zero edits to `Tree.cpp` bodies**. `ViewProviderOriginGroupExtension::constructChildren` [VERIFIED: src/Gui/ViewProviderOriginGroupExtension.cpp:57-79] explicitly forces the Origin node first; `ViewProviderSketchBased::claimChildren` [VERIFIED: src/Mod/PartDesign/Gui/ViewProviderSketchBased.cpp:77-86] nests the profile sketch under its feature. This makes **D-03 (reuse-and-wrap) overwhelmingly likely to PASS** — the remaining work is *scoping* the tree to one active Body and *restyling* it, both achievable from the FreeWorks layer via a thin `Gui::TreeWidget` subclass + a `QStyledItemDelegate` (D-08 label remap) + a Gui-only greying mechanism (D-06).

The rollback engine (TREE-02) is equally well-supported. `Body.Tip` is an existing persisted `App::PropertyLink` [VERIFIED: src/Mod/Part/App/BodyBase.h:54]; `Body::mustExecute()` returns 1 when `Tip.isTouched()` [VERIFIED: src/Mod/PartDesign/App/Body.cpp:94-100], so a transactioned `Tip.setValue(feature)` followed by recompute *is* the true suppress-below behavior — exactly what `CmdPartDesignMoveTip::activated` does [VERIFIED: src/Mod/PartDesign/Gui/CommandBody.cpp:671-745]. Insert-at-bar reuses `Body::insertObject(feature, target, after)` [VERIFIED: src/Mod/PartDesign/App/Body.cpp:275-321]. The 3D "rolled-back result" is rendered by the existing `ViewProviderBody` tip display [VERIFIED: src/Mod/PartDesign/Gui/ViewProviderBody.cpp:307-332, 602-641].

Drag-reorder validity (TREE-04) is also reuse. `TreeWidget::sortDroppedObjects` reorders the body's `Group` `PropertyLinkList` [VERIFIED: src/Gui/Tree.cpp:3260-3273], gated by the dependency-aware `canDragObjectToTarget`/`canDropObjectEx` ViewProvider hooks [VERIFIED: src/Gui/ViewProvider.h:444-493] with PartDesign's `ViewProviderBody` overrides [VERIFIED: src/Mod/PartDesign/Gui/ViewProviderBody.cpp:593-600]. SolidWorks BLOCKS invalid reorders with "Cannot reorder. Change would put child feature before parent feature" [CITED: forum.solidworks.com/thread/24847] — confirming D-11's BLOCK decision matches SW feel exactly.

**Primary recommendation:** Build `FwFeatureTree : Gui::TreeWidget` (thin subclass, additive, `src/Gui/FreeWorks/`), mount it in the `Fw_FeatureManager` dock, reuse `Body.Tip`/`Body::insertObject` for rollback, reuse the stock DnD/F2 machinery verbatim, and add ONLY: (1) active-Body scoping, (2) a `QStyledItemDelegate` for the Front/Top/Right plane label remap, (3) a Gui-only below-tip greying, (4) the rollback-bar affordance widget. Run the D-03 spike first to confirm the subclass can scope+restyle without touching `Tree.cpp`.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Tree structure (Origin/features/sketches) | Gui (ViewProvider `claimChildren`) | — | Already produced by stock ViewProviders; the tree only *renders* the DOM it observes |
| Tree widget hosting + SW restyle | Gui (FreeWorks `FwFeatureTree`) | — | Pure presentation; subclass of `Gui::TreeWidget` |
| Active-Body scoping | Gui (FreeWorks, via Gui signals) | — | `Gui::Application::signalActivatedObject`/`signalInEdit`; type-name literal `"PartDesign::Body"` (no module link) |
| Rollback state (suppress-below) | App (`Body.Tip` PropertyLink) | Gui (bar widget) | Tip is the single persisted source of truth; bar is Gui-only affordance |
| Recompute on rollback | App (`Document::recompute` via `mustExecute`) | — | Driven by `Tip.isTouched()` |
| 3D suppress-below display | Gui (`ViewProviderBody` tip display) | — | Existing `showTip`/`DisplayModeBody` already shows the tip shape |
| Drag-reorder validity gate | Gui (`ViewProvider` DnD hooks) | App (`Group` PropertyLinkList) | Dependency rules encoded in `canDragObjectToTarget`/`canDropObjectEx` |
| Reorder commit | App (`Group.setValue` in transaction) | Gui (`sortDroppedObjects`) | Clean undo/redo via single transaction |
| F2 rename | App (`Label.setValue` in transaction) | Gui (`editItem`/`setData`) | Self-contained in `DocumentObjectItem::setData` |

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** Reuse-first — wrap and restyle FreeCAD's existing `Gui::TreeWidget`, re-hosted in the left `Fw_FeatureManager` dock. The stock tree provides for free: DnD reorder (`startDrag`/`dropMimeData`/`sortDroppedObjects`), F2 inline rename, `claimChildren` nesting, SelectionObserver wiring.
- **D-02:** Achieve SW presentation via FreeWorks-side configuration/ViewProvider hooks, NOT by editing `src/Gui/Tree.cpp` bodies. Prefer (1) a thin `FwFeatureTree : Gui::TreeWidget` subclass, or (2) FreeWorks ViewProvider/filter configuration. Any unavoidable shared touch gets `// SW-FORK HOOK`.
- **D-03 (PLAN-TIME SPIKE):** Before the full build, confirm the existing `Gui::TreeWidget` can be (a) scoped to the active Body, (b) reordered to show Origin/planes at the top, (c) restyled SW-like WITHOUT editing `Tree.cpp` bodies. PASS → reuse-and-wrap committed. FAIL → fall back to a thin subclass (still reusing DnD/rename/transaction machinery). From-scratch is last resort.
- **D-04:** The rollback bar drives the REAL `Body.Tip` (true suppress-below + recompute), reusing the `PartDesign_MoveTip` path (`CommandBody.cpp:657`, `Tip.setValue`); insert-at-bar reuses `Body::insertObject(feature, target, after)`. A cosmetic-only marker is REJECTED.
- **D-05:** Reconcile SC2 — the rollback *bar widget* (position, drag affordance, greying) is Gui-only; the *model state* uses the existing persisted `Body.Tip` App property. NO new persisted rollback property, NO parallel suppression. `Tip` is the single source of truth.
- **D-06:** Features below the rollback bar are visually de-emphasized (greyed) in the tree (Gui-only styling); the 3D view shows the rolled-back result via existing `ViewProviderBody` tip display (`showTip`/`DisplayModeBody`). Dragging the bar back restores forward — reversible.
- **D-07 (in scope):** active-Body-scoped tree; top Origin node (3 planes + 3 axes + point via `App::Origin`); features in creation order; sketches nested under consuming feature (reuse `ViewProviderSketchBased::claimChildren`).
- **D-08 (in scope):** display origin planes under SolidWorks display names ("Front/Top/Right Plane") as a Gui-only label mapping over FreeCAD's XY/XZ/YZ planes — underlying objects unchanged. Structural/labeling, not icon art.
- **D-09 (deferred):** SW icon artwork, exact colors/fonts → Phase 7. Extra SW nodes (Material, Annotations, Sensors, Equations, Mates, Lights/Cameras) → out of scope. Phase 3 ships Origin + features + sketches only.
- **D-10:** Reuse FreeCAD's dependency-aware DnD machinery (`TreeWidget` DnD + ViewProvider gates `canDragObjectToTarget`/`canDropObjectEx`, with `ViewProviderBody` overrides).
- **D-11:** Show a visible insertion-line affordance during drag; on an invalid reorder, BLOCK the drop (forbidden/no-drop cursor, no commit). Allow-and-flag-error is REJECTED.
- **D-12:** Valid reorders commit through a single document transaction (`Document::openTransaction`/`commitTransaction`) via `Body::insertObject` / the Group `PropertyLinkList` reorder.
- **D-13:** Reuse the existing F2 rename path verbatim (`relabelObjectAction`, `Key_F2` → `editItem` → `DocumentObjectItem::setData` → `Label.setValue`, transaction-wrapped). Target the user-facing `Label`. Only work: ensure it is wired/active in the `Fw_FeatureManager`-hosted tree.

### Claude's Discretion
- Exact widget seam to mount in the dock (subclass vs configured instance) — resolved by the D-03 spike.
- Precise greying mechanism for below-bar rows (stylesheet vs `QPalette`/item flags) — but no inline hex (Phase 7 owns color; palette-only rule per Phase 2).
- Whether the rollback bar is a custom drawn row/separator in the tree vs an overlay widget — planner's call, provided it drives `Body.Tip` per D-04.

### Deferred Ideas (OUT OF SCOPE)
- Double-click feature → edit in PropertyManager (TREE-03): Phase 4 (needs PropertyManager container).
- SW icon artwork, colors, fonts: Phase 7 (Visual Theme).
- Extra SW tree nodes (Material, Annotations, Sensors, Equations, Mates, Lights/Cameras): out of scope this milestone.
- Multi-body / assembly tree semantics: out of scope (single active Body this phase).
- Folder/grouping features inside the tree (SW folders): defer unless a later phase requires it.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| TREE-01 | Left-docked FeatureManager tree: features in creation order, origin/planes node, sketches nested under features | Stock `Gui::TreeWidget` + `ViewProviderOriginGroupExtension::constructChildren` (Origin-on-top, src/Gui/ViewProviderOriginGroupExtension.cpp:57) + `ViewProviderSketchBased::claimChildren` (sketch nesting, ViewProviderSketchBased.cpp:77). Mount in `Fw_FeatureManager` (FwLayout.cpp:125). Plane label remap via `QStyledItemDelegate` over `PlaneRoles` (Datums.h:204). |
| TREE-02 | Drag a rollback bar to roll model back to an earlier feature state and insert/edit mid-history | `Body.Tip` (BodyBase.h:54) + `Body::mustExecute` on `Tip.isTouched` (Body.cpp:94) + `CmdPartDesignMoveTip` path (CommandBody.cpp:671) for set-tip; `Body::insertObject` (Body.cpp:275) for insert-at-bar; `ViewProviderBody` tip display (ViewProviderBody.cpp:307,602) for 3D suppress-below. |
| TREE-04 | Drag features up/down to reorder history, with validity feedback | `TreeWidget::sortDroppedObjects` → `Group` PropertyLinkList reorder (Tree.cpp:3260) + dependency gates `canDragObjectToTarget`/`canDropObjectEx` (ViewProvider.h:450,483) with `ViewProviderBody` overrides (ViewProviderBody.cpp:593). BLOCK invalid drop (D-11) matches SW "child before parent" block. |
| (F2 half of TREE-03) | F2 rename | `DocumentObjectItem::setData` transactioned `Label.setValue` (Tree.cpp:6536-6556); `relabelObjectAction` bound to `Key_F2` (Tree.cpp:607-614). Reuse verbatim (D-13). |
</phase_requirements>

## Project Constraints (from CLAUDE.md)

- **App/Gui separation:** FreeWorks code (Gui layer) must NOT include `App/` headers for App-layer mutation, and must NOT hard-link PartDesign/Sketcher modules. Phase 2 precedent: reference modules only by command-ID / type-name *string literals*, never by include/link. The active-Body scoping (D-07) MUST use Gui-layer signals (`Gui::Application::signalActivatedObject`/`signalInEdit`) + a type-name literal `"PartDesign::Body"` check, OR fire rollback through the existing command/Python path — NOT a `#include <Mod/PartDesign/...>` + link.
- **No document mutation during recompute.** All App-state changes (Tip move, reorder, rename) go through `Document::openTransaction`/`commitTransaction` (Property setters), never direct field writes.
- **Coin3D ref/unref discipline** — N/A for the tree widget (pure Qt), but relevant if any 3D preview is touched (it is not in this phase; the existing ViewProviderBody owns 3D).
- **Additive module discipline:** new code under `src/Gui/FreeWorks/`, compiled into FreeCADGui via `target_sources(FreeCADGui PRIVATE ...)`, namespace `FreeWorksGui`, `Fw` prefix. Exactly the established Phase 1/2 pattern.
- **No "SolidWorks" identifier in source** except the allow-listed `Gui::SolidWorksNavigationStyle`. The Front/Top/Right plane labels are *user-facing tr() strings*, not identifiers — but verify the leak-grep (`tools/fw-string-leak-grep.sh`) allow-lists or that the strings live in `tr()` display text only. SW plane names as displayed labels are user-facing parity text; confirm the grep scope (it targets identifiers/comments, not necessarily `tr()` literals — VERIFY at plan time).
- **Pre-commit:** clang-format (LLVM, 4-space, 100 col), headers `.h`, classes PascalCase, private members `_lowerCamelCase`.

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Qt Widgets (`QTreeWidget`, `QStyledItemDelegate`, `QApplication`) | 6.8.x | Tree widget base, item rendering, label remap | The base class `Gui::TreeWidget` already extends `QTreeWidget` [VERIFIED: src/Gui/Tree.h:60] |
| `Gui::TreeWidget` (FreeCAD) | main (1.2.0-dev) | Reused tree engine: DnD, F2, claimChildren, SelectionObserver | The keystone reuse target per D-01 [VERIFIED: src/Gui/Tree.h:60-341] |
| `PartDesign::Body` / `Body.Tip` (FreeCAD) | main | Rollback engine (Tip + recompute) | True suppress-below mechanism per D-04 [VERIFIED: src/Mod/Part/App/BodyBase.h:54, Body.cpp:94] |
| GoogleTest (GTest) | bundled | Headless command-ID/structure resolution tests | Phase 1/2 `Gui_tests_run` pattern [VERIFIED: tests/src/Gui/FwRibbon.cpp] |
| QTest (`QTEST_MAIN` + offscreen) | 6.8.x | QWidget construction / tree-build tests | Phase 2 `setup_qt_test` pattern [VERIFIED: tests/src/Gui/FwRibbonWidget.cpp, CMakeLists.txt:23] |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `Gui::Application` signals (`signalActivatedObject`, `signalInEdit`, `signalActiveDocument`) | main | Observe active document/body (observe-the-DOM) | Active-Body scoping (D-07) without App-layer links [VERIFIED: src/Gui/Application.h:130,142,154] |
| `App::Origin` / `OriginGroupExtension` | main | Groups 3 planes + 3 axes + point | Origin node (D-07); roles in `PlaneRoles`/`AxisRoles`/`PointRoles` [VERIFIED: src/App/Datums.h:202-206] |
| `Gui::DockWindowManager::registerDockWindow` | main | Mount content under `Fw_FeatureManager` | Replace the Phase-1 placeholder [VERIFIED: src/Gui/FreeWorks/FwLayout.cpp:101,125] |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Thin `FwFeatureTree : Gui::TreeWidget` subclass | Configured stock `TreeWidget` instance (no subclass) | Stock instance can be mounted but cannot override `drawRow` (rollback bar render) or set a custom delegate without a subclass; D-08 label remap + D-06 greying + D-11 insertion-line all want a subclass seam. Subclass is the lower-risk default; D-03 spike confirms. |
| Subclass overriding presentation | Edit `Tree.cpp` bodies | REJECTED by D-02 (shared-file edits cause merge pain). |
| Reuse `Body.Tip` for rollback | New Gui-only cosmetic marker | REJECTED by D-04/D-05 (doesn't recompute → fails SC5 "feel"; or new persisted property pollutes `.FCStd`). |
| Reuse `sortDroppedObjects` reorder | Hand-rolled reorder | REJECTED by D-10 — would re-implement dependency gates and lose clean undo. |

**Installation:** No new packages. All dependencies are in-tree FreeCAD/Qt6. The phase adds source files to `src/Gui/FreeWorks/` via `target_sources(FreeCADGui PRIVATE ...)`.

## Package Legitimacy Audit

> Not applicable. This phase installs **no external packages** — it reuses in-tree FreeCAD Gui/App classes and Qt 6.8 Widgets only. No npm/PyPI/crates dependency is introduced. (Confirmed: `src/Gui/FreeWorks/CMakeLists.txt` adds sources to FreeCADGui; no `target_link_libraries` to third-party.)

## Architecture Patterns

### System Architecture Diagram

```
                    ┌─────────────────────────────────────────────────┐
   User drags       │            Fw_FeatureManager dock                │
   rollback bar ───▶│  ┌───────────────────────────────────────────┐  │
   / drag-reorders  │  │  FwFeatureTree : Gui::TreeWidget           │  │
   / presses F2     │  │  (additive, FreeWorksGui)                  │  │
                    │  │   • scope to active Body (Gui signals)     │  │
                    │  │   • QStyledItemDelegate → plane label remap│  │
                    │  │   • drawRow / overlay → rollback bar        │  │
                    │  │   • Gui-only greying of below-tip rows      │  │
                    │  └──────────────┬────────────────────────────┘  │
                    └─────────────────┼───────────────────────────────┘
                                      │  observes (SelectionObserver,
                                      │   ViewProvider signals)
              ┌───────────────────────▼──────────────────────────────┐
              │           Gui::Document / ViewProviders                │
              │  ViewProviderBody.updateData(Tip) ─▶ tip icon + 3D     │
              │  ViewProviderOriginGroupExtension.claimChildren ─▶     │
              │     Origin-first ordering                              │
              │  ViewProviderSketchBased.claimChildren ─▶ sketch nest  │
              │  canDragObjectToTarget / canDropObjectEx ─▶ DnD gate   │
              └───────────────────────┬────────────────────────────────┘
                  transactioned writes │ (openTransaction/commitTransaction)
                                       ▼
              ┌────────────────────────────────────────────────────────┐
              │              App::Document (single source of truth)      │
              │  Body.Tip (PropertyLink) ── set ─▶ mustExecute()==1 ─▶   │
              │     recompute ─▶ suppress-below result                   │
              │  Body.Group (PropertyLinkList) ── reorder ─▶ rebuild     │
              │  feature.Label (PropertyString) ── F2 rename             │
              │  Body::insertObject(feature,target,after) ── insert-at-bar│
              └────────────────────────────────────────────────────────┘
```

Trace the rollback use case: user drags bar up → FwFeatureTree resolves the feature under the bar → opens a transaction → sets `Body.Tip = thatFeature` (via the `MoveTip`/Python command path) → `Tip.isTouched()` makes `mustExecute` return 1 → recompute → `ViewProviderBody.updateData` repaints tip icon and 3D shows the rolled-back shape → FwFeatureTree greys the below-tip rows (Gui-only). Drag back down → set Tip forward → reversible.

### Recommended Project Structure
```
src/Gui/FreeWorks/
├── FwFeatureTree.h/.cpp     # FwFeatureTree : Gui::TreeWidget — the mounted tree
├── FwFeatureTreeDelegate.h/.cpp  # QStyledItemDelegate: Front/Top/Right plane label remap (D-08)
├── FwRollbackBar.h/.cpp     # rollback-bar affordance (overlay or drawn row); drives Body.Tip (D-04)
├── FwLayout.cpp             # MODIFY: replace Fw_FeatureManager placeholder with FwFeatureTree
└── CMakeLists.txt           # MODIFY: add new sources to FreeCADGui (additive)

tests/src/Gui/
├── FwFeatureTree.cpp        # GTest (Gui_tests_run): structure/scoping/Tip-logic (pure-logic)
└── FwFeatureTreeWidget.cpp  # QTEST_MAIN offscreen: widget construction, delegate label remap
```

### Pattern 1: Reuse Body.Tip as the rollback engine (D-04)
**What:** Setting `Body.Tip` to an earlier feature, inside a transaction, then recomputing, is the canonical suppress-below.
**When to use:** Every rollback-bar drag commit and "Roll to End."
**Example:**
```cpp
// Source: src/Mod/PartDesign/Gui/CommandBody.cpp:730-744 (CmdPartDesignMoveTip::activated)
openCommand(QT_TRANSLATE_NOOP("Command", "Move tip to selected feature"));
if (selFeature == body) {
    FCMD_OBJ_CMD(body, "Tip = None");          // roll to base
} else {
    FCMD_OBJ_CMD(body, "Tip = " << getObjectCmd(selFeature));
    FCMD_OBJ_SHOW(selFeature);                  // show only the tip feature
}
updateActive();   // triggers recompute; Body::mustExecute() returns 1 on Tip.isTouched()
```
**Note for FreeWorks (App/Gui separation):** Prefer firing this through the existing `PartDesign_MoveTip` command (`CommandManager::getCommandByName("PartDesign_MoveTip")` + a selection of the target feature) OR the Python `obj.Tip = ...` command string — NOT a direct C++ `Body::Tip.setValue` call that would require linking PartDesign into FreeWorks. This mirrors Phase 2's "reference by command-ID string, never link" rule.

### Pattern 2: Origin-on-top + sketch nesting are FREE (D-07)
**What:** The tree structure SW users expect is already emitted by ViewProviders.
**When to use:** No code needed beyond mounting the tree scoped to the Body — the structure is automatic.
**Example:**
```cpp
// Source: src/Gui/ViewProviderOriginGroupExtension.cpp:57-75 (Origin forced first)
App::DocumentObject* originObj = group->Origin.getValue();
if (originObj) {
    std::vector<App::DocumentObject*> rv;
    rv.push_back(originObj);                    // Origin must be first
    std::copy(children.begin(), children.end(), std::back_inserter(rv));
    return rv;
}
// Source: src/Mod/PartDesign/Gui/ViewProviderSketchBased.cpp:77-86 (sketch nested under feature)
App::DocumentObject* sketch = getObject<PartDesign::ProfileBased>()->Profile.getValue();
if (sketch && !sketch->isDerivedFrom<PartDesign::Feature>())
    temp.push_back(sketch);                     // claimed as child of the feature
```

### Pattern 3: Plane label remap via QStyledItemDelegate (D-08)
**What:** Tree item text comes from `obj->Label.getValue()` set via `item->setText(0, ...)` [VERIFIED: src/Gui/Tree.cpp:4568]. A custom delegate (the tree already uses `TreeWidgetItemDelegate`) can remap *display text only* for the three origin planes, keyed on the internal `PlaneRoles` name, without mutating the object.
**When to use:** Rendering origin planes as Front/Top/Right.
**Mapping (SW ↔ FreeCAD):**
| SolidWorks name | FreeCAD role (Datums.h:204) | FreeCAD label (Datums.cpp:270-272) |
|-----------------|------------------------------|-------------------------------------|
| Front Plane | `XY_Plane` | "XY-plane" |
| Top Plane | `XZ_Plane` | "XZ-plane" |
| Right Plane | `YZ_Plane` | "YZ-plane" |
**Note:** SW Front = XY (view down -Z), Top = XZ, Right = YZ — this is the standard correspondence [ASSUMED — verify against a live SW install or pinned Help during the parity-user check; the geometric mapping is training knowledge, not confirmed from official Help this session].

### Pattern 4: Reorder through the Group PropertyLinkList in one transaction (D-12)
**What:** `sortDroppedObjects` reorders the body's `Group` property; for an object target it sets `Group` directly.
**Example:**
```cpp
// Source: src/Gui/Tree.cpp:3260-3273
auto propGroup = freecad_cast<App::PropertyLinkList*>(targetObj->getPropertyByName("Group"));
if (!propGroup) return;
objList = propGroup->getValue();
sortIntoList(objList);          // place dragged objects relative to drop target
propGroup->setValue(sortedObjList);   // clean undo/redo when wrapped in a transaction
```

### Anti-Patterns to Avoid
- **Editing `Tree.cpp` bodies for the SW view:** REJECTED by D-02. Use a subclass + delegate + overlay instead.
- **A new persisted rollback property:** REJECTED by D-05 — pollutes `.FCStd`, can diverge from `Tip`.
- **Linking PartDesignGui/Sketcher into FreeWorks:** breaks App/Gui separation + merge discipline. Use command-ID/type-name string literals + Gui signals (Phase 2 precedent).
- **Allow-and-flag invalid reorder:** REJECTED by D-11 — leaves a broken recompute state; BLOCK instead (matches SW).
- **Direct `Body::Tip.setValue` from Gui C++:** prefer the command/Python path to avoid the module link.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Tree DnD reorder | Custom drag/drop | `TreeWidget::startDrag`/`dropMimeData`/`sortDroppedObjects` | Encodes treeRank, Group reorder, recompute-on-drop, transaction [VERIFIED: Tree.cpp:3222-3308] |
| Dependency validity gate | Custom dependency graph | `canDragObjectToTarget`/`canDropObjectEx` + `ViewProviderBody` overrides | Solid-feature ordering rules already encoded [VERIFIED: ViewProvider.h:450,483; ViewProviderBody.cpp:593] |
| Suppress-below + recompute | Manual feature hiding | `Body.Tip` + recompute (`mustExecute` on `Tip.isTouched`) | True parametric rollback, persisted, reversible [VERIFIED: Body.cpp:94-100] |
| Insert feature mid-history | Custom Group splicing | `Body::insertObject(feature, target, after)` | Handles BaseFeature rerouting + origin relink [VERIFIED: Body.cpp:275-338] |
| F2 inline rename | Custom edit widget | `editItem` → `DocumentObjectItem::setData` → transactioned `Label.setValue` | Self-contained, transaction-wrapped [VERIFIED: Tree.cpp:6536-6556] |
| Origin/planes node | Manually build node | `App::Origin` + `ViewProviderOriginGroupExtension` (Origin forced first) | Automatic [VERIFIED: ViewProviderOriginGroupExtension.cpp:57] |
| Sketch-under-feature nesting | Manual parent mapping | `ViewProviderSketchBased::claimChildren` | Automatic [VERIFIED: ViewProviderSketchBased.cpp:77] |
| 3D suppress-below render | Custom scene-graph edits | `ViewProviderBody` tip display (`showTip`/`DisplayModeBody`) | Already renders the tip shape [VERIFIED: ViewProviderBody.cpp:307,602] |

**Key insight:** Phase 3 is ~90% reuse-and-mount. The genuinely *new* code is the FreeWorks-side presentation shell (subclass + delegate + greying + rollback-bar affordance) and the active-Body scoping. Every behavior that touches the document already exists, is transactioned, and is recompute-aware.

## Runtime State Inventory

> Not a rename/refactor/migration phase — this is net-new Gui presentation that reuses existing engines. No stored data, live-service config, OS-registered state, secrets, or build artifacts carry phase-specific state to migrate. The only persisted state touched is the **existing** `Body.Tip` property (no new key, no migration). **None — verified by reading the decisions (D-05: no new persisted property) and the Body.Tip reuse path.**

## Common Pitfalls

### Pitfall 1: Scoping the tree to one Body without an App-layer link
**What goes wrong:** The naive way to "scope to active Body" is to call `PartDesignGui::getBody(...)` [src/Mod/PartDesign/Gui/Utils.h:55] — but that lives in PartDesignGui and would force a module link, breaking App/Gui separation and the upstream-merge discipline.
**Why it happens:** The clean active-body accessor is in the wrong layer for FreeWorks.
**How to avoid:** Observe via Gui-layer signals: `Gui::Application::signalActivatedObject`/`signalInEdit`/`signalActiveDocument` [VERIFIED: src/Gui/Application.h:130,142,154], and identify a Body by the type-name *string literal* `"PartDesign::Body"` (`obj->getTypeId().getName()` comparison) — the exact pattern Phase 2 used for the Sketcher type-name (`"SketcherGui::ViewProviderSketch"`, STATE.md [02-04]).
**Warning signs:** A `#include <Mod/PartDesign/...>` or `target_link_libraries(... PartDesignGui)` appearing in `src/Gui/FreeWorks/`.

### Pitfall 2: The TreeWidget subclass surface is thinner than it looks
**What goes wrong:** Most tree presentation logic lives in `DocumentItem`/`DocumentObjectItem` (item creation, `populateItem`, `createNewItem`) which are NOT virtual and are private/protected with `TreeWidget` as `friend` [VERIFIED: src/Gui/Tree.h:328-331, 456-482]. A subclass cannot easily override how individual items are built.
**Why it happens:** The reusable virtual seams on `TreeWidget` itself are: `drawRow` (protected virtual, Tree.cpp:3310), the DnD overrides (`startDrag`/`dropMimeData`/`dragMoveEvent`/`dropEvent`), `keyPressEvent`, `eventFilter`, plus `setItemDelegate` (from QTreeWidget). Per-item construction is not virtual.
**How to avoid:** Drive presentation through (a) `drawRow` override for the rollback-bar drawn row, (b) a `QStyledItemDelegate` for plane-label remap + below-tip greying, (c) `setItemDelegate`/stylesheet for SW look, (d) post-populate item-flag/`QPalette` adjustment via the document-changed/recomputed signals — NOT by overriding item creation. **This is the precise question the D-03 spike must answer:** can scoping + Origin-on-top + restyle be done from these seams alone? (Origin-on-top is already free via claimChildren, so the spike's real risk is *scoping to one Body* and *greying* — both look achievable.)
**Warning signs:** Finding yourself wanting to override `DocumentItem::createNewItem` → that's a FAIL signal pushing toward the (still additive) heavier subclass or a filter approach.

### Pitfall 3: Greying below-tip rows must not mutate App state
**What goes wrong:** Tempting to set `feature.Visibility = false` to grey/hide below-tip features — but that mutates the document and pollutes undo/`.FCStd`.
**Why it happens:** Visibility is the obvious lever.
**How to avoid:** Grey purely in the Gui via the delegate / `Qt::ForegroundRole` / `QPalette::Disabled` / item flags on the tree items — never touch the object. The 3D suppress-below is already handled by `ViewProviderBody`'s tip display, which `Body.Tip` drives automatically. (D-06 explicitly says greying is Gui-only.)
**Warning signs:** Any `Visibility.setValue` or property write in the greying code path.

### Pitfall 4: "Roll to End" / reversibility must restore the forward Tip
**What goes wrong:** After rolling back, the user expects dragging the bar down (or "Roll to End") to restore the latest feature as tip. If the bar only ever moves Tip backward, it's not reversible (fails SC5).
**Why it happens:** Forgetting that Tip can move both directions.
**How to avoid:** "Roll to End" = set Tip to the last solid feature (`Body::getNextSolidFeature` chain to the end, or simply the last `isSolidFeature` in `Group`). Dragging down sets Tip forward. All within one transaction each.
**Warning signs:** No path to re-set Tip to the final feature.

### Pitfall 5: Datums/sketches are skipped by the solid-feature tip logic
**What goes wrong:** `Body::isSolidFeature` returns false for datums and sketches [VERIFIED: src/Mod/PartDesign/App/Body.cpp:176-194], and `Tip` may only point to a PartDesign solid feature (the MoveTip command warns "Only a solid feature can be the tip" [VERIFIED: CommandBody.cpp:712-721]). A rollback bar dropped on a sketch/datum row must resolve to the appropriate solid boundary, not set Tip to the sketch.
**Why it happens:** The tree shows sketches/datums interleaved; the bar position maps to a feature.
**How to avoid:** When the bar lands on/above a non-solid row, snap the effective Tip to the nearest preceding solid feature (`getPrevSolidFeature`). SW's rollback bar similarly rolls to feature boundaries.
**Warning signs:** A `Tip = <sketch>` command — will be rejected/warn.

## Code Examples

### Mount the tree in the Fw_FeatureManager dock (replace placeholder)
```cpp
// Source pattern: src/Gui/FreeWorks/FwLayout.cpp:101,125 (placeholder registration today)
// Phase 3 replaces makePlaceholder(...) for "Fw_FeatureManager" with an FwFeatureTree instance.
manager->registerDockWindow("Fw_FeatureManager", new FwFeatureTree("FwFeatureManager", parent));
// FwFeatureTree(const char* name, QWidget* parent) : Gui::TreeWidget(name, parent) { ... }
// TreeWidget's ctor is public and takes (const char* name, QWidget* parent) [Tree.h:65].
```

### Reorder commit in a single transaction (D-12)
```cpp
// Source: src/Gui/Tree.cpp:3260-3273 + transaction discipline (Tree.cpp:6549 pattern)
App::Document* doc = targetObj->getDocument();
doc->openTransaction("Reorder feature");
auto propGroup = freecad_cast<App::PropertyLinkList*>(targetObj->getPropertyByName("Group"));
propGroup->setValue(sortedObjList);
doc->commitTransaction();   // clean undo/redo
```

### F2 rename (already wired — verify active in mounted tree, D-13)
```cpp
// Source: src/Gui/Tree.cpp:607-614 (binding) + 6536-6556 (transactioned commit)
// relabelObjectAction->setShortcut(Qt::Key_F2);  // (Qt::Key_Return on macOS)
// onRelabelObject() -> editItem(currentItem()) -> DocumentObjectItem::setData(EditRole)
//   -> doc->openTransaction("Rename ..."); label.setValue(...); doc->commitTransaction();
// No new code: confirm the action is installed on FwFeatureTree (inherited from TreeWidget).
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `Body::onChanged` Tip auto-repair (commented-out legacy block) | `mustExecute()` returns 1 on `Tip.isTouched()`; Tip repaired only on remove | current main | Tip change → recompute is the supported path; no custom touch logic needed [VERIFIED: Body.cpp:80-100,360-368] |
| Hard-coded tree item text | Item text from `Label`, restylable via `QStyledItemDelegate` (`TreeWidgetItemDelegate` already in use) | current main | Delegate is the supported D-08 remap seam [VERIFIED: Tree.h:50, Tree.cpp:4568] |

**Deprecated/outdated:** None relevant. The legacy commented-out `Body::onChanged` tip block (Body.cpp:~60-92) is dead code — do not revive it; use `mustExecute`/`Tip.setValue`.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | SW Front Plane = FreeCAD XY, Top = XZ, Right = YZ (geometric correspondence) | Pattern 3 (D-08 mapping) | Wrong plane labels → SW users confused; LOW effort to flip the mapping. Verify against live SW / pinned Help in the parity-user check. |
| A2 | `tools/fw-string-leak-grep.sh` allows SW plane names inside `tr()` display strings (not flagged as the "SolidWorks" identifier) | Project Constraints | If the grep flags "Front Plane"/etc., a tr()-scope exemption or comment is needed; LOW. The forbidden token is "SolidWorks", which these strings do not contain — likely fine. |
| A3 | Firing `PartDesign_MoveTip` via `CommandManager::getCommandByName` from FreeWorks (selecting the target feature first) sets Tip without a module link | Pattern 1 note | If the command requires a selection context FreeWorks can't supply, fall back to a Python `obj.Tip = ...` `doCommand` string. Both avoid linking. MEDIUM (selection plumbing). |
| A4 | A `QStyledItemDelegate` + `drawRow` override + post-populate `QPalette`/flags can deliver D-06 greying + D-08 remap + D-11 insertion-line WITHOUT editing Tree.cpp | Pitfall 2 / Spike | This is exactly what the D-03 spike must prove. If a needed item-construction override is non-virtual, fall back to the heavier-but-still-additive subclass approach (D-03 FAIL branch). MEDIUM. |

**If this table is empty:** It is not — A1–A4 need confirmation (A1 in the parity-user check; A2–A4 during the D-03 spike / plan-time).

## Open Questions

1. **Can active-Body scoping be done from a single-document tree filter, or does it need a dedicated scoped tree?**
   - What we know: `TreeWidget` shows all documents/objects; `DocumentItem` builds per-document. ViewProviders already nest correctly under the Body.
   - What's unclear: whether scoping to *only* the active Body's subtree is a filter (hide everything not under the active Body) or requires mounting a tree rooted at the Body. Greying/hiding non-Body items must be Gui-only.
   - Recommendation: Spike both — (a) filter the stock tree to the active Body subtree via item visibility, (b) confirm Origin-on-top + sketch nesting render correctly within that scope. Likely (a) suffices since structure is free.

2. **Does the leak-grep allow user-facing SW plane label strings?**
   - What we know: forbidden identifier is "SolidWorks"; plane labels are "Front Plane" etc. (no "SolidWorks" substring).
   - Recommendation: Confirm `tools/fw-string-leak-grep.sh` scope at plan time; the strings should pass since they don't contain the forbidden token.

3. **Rollback-bar widget form (drawn row vs overlay) — planner's call (D-discretion).**
   - Recommendation: A `drawRow` override (drawn separator at the tip boundary) is the lowest-risk, scroll-coherent option; an overlay widget needs manual repositioning on scroll. Decide in the spike.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| Qt 6.8 Widgets | Tree widget, delegate | ✓ (in-tree dep) | 6.8.x | — |
| FreeCAD build tree (configured) | Compile + ctest + live GUI | ✗ (not in this env) | — | Headless tests authored compile-intended; live ride CI matrix (Phase 1/2 precedent) |
| GTest / QTest harness | Headless tests | ✓ (source present) | bundled | — |
| Live SW install (parity check) | SC5 feel validation, A1 plane mapping | ✗ | — | Deferred to daily-SW-user parity track (PITFALLS.md Pitfall 7) + a SPIKE_LIVE_CHECKLIST |

**Missing dependencies with no fallback:** None blocking. **With fallback:** No build tree → authored tests + deferred live checklist (continues the Phase 1/2 verification-deferral pattern); no SW install → parity-user track.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | GoogleTest (`Gui_tests_run`) for pure-logic + QTest (`QTEST_MAIN`, offscreen) for widget construction |
| Config file | `tests/src/Gui/CMakeLists.txt` (add `FwFeatureTree.cpp` to `Gui_tests_run`; `setup_qt_test(FwFeatureTreeWidget)`) [VERIFIED: CMakeLists.txt:7,23] |
| Quick run command | `ctest -R "Gui_tests_run"` |
| Full suite command | `ctest -R "Gui_tests_run|FwFeatureTreeWidget_Tests_run"` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| TREE-01 | Tree mounts in Fw_FeatureManager; structure = Origin-first + creation order + nested sketches | QTest (offscreen) build + structure assert | `ctest -R FwFeatureTreeWidget_Tests_run` | ❌ Wave 0 |
| TREE-01 | Plane label remap: XY_Plane → "Front Plane" etc. (delegate display text) | QTest (offscreen) | `ctest -R FwFeatureTreeWidget_Tests_run` | ❌ Wave 0 |
| TREE-01 | Active-Body scoping resolves a `"PartDesign::Body"` type-name (no module link) | GTest (logic) | `ctest -R Gui_tests_run` | ❌ Wave 0 |
| TREE-02 | Set-tip-to-earlier-feature → `Tip.isTouched()` → `mustExecute()==1` (rollback logic) | GTest (logic, build a Body + features, set Tip, assert mustExecute) | `ctest -R Gui_tests_run` | ❌ Wave 0 |
| TREE-02 | Insert-at-bar via `Body::insertObject(target, after)` places feature correctly in Group | GTest (logic) | `ctest -R Gui_tests_run` | ❌ Wave 0 |
| TREE-02 | "Roll to End" sets Tip to last solid feature; reversible | GTest (logic) | `ctest -R Gui_tests_run` | ❌ Wave 0 |
| TREE-02 | 3D suppress-below + drag feel (look like SW) | **manual / live-only** | SPIKE_LIVE_CHECKLIST + parity-user track | n/a (deferred) |
| TREE-04 | Valid reorder commits via Group PropertyLinkList in one transaction; clean undo | GTest (logic, reorder Group, assert order + undo restores) | `ctest -R Gui_tests_run` | ❌ Wave 0 |
| TREE-04 | Invalid reorder (child before parent) is BLOCKED by `canDragObjectToTarget`/`canDropObjectEx` (no commit) | GTest (logic, assert gate returns false) | `ctest -R Gui_tests_run` | ❌ Wave 0 |
| TREE-04 | Insertion-line affordance + forbidden cursor on invalid drop | **manual / live-only** | SPIKE_LIVE_CHECKLIST | n/a (deferred) |
| TREE-03 (F2) | F2 → editItem → transactioned `Label.setValue`; action installed on FwFeatureTree | QTest (offscreen) assert action + shortcut present | `ctest -R FwFeatureTreeWidget_Tests_run` | ❌ Wave 0 |

### Sampling Rate
- **Per task commit:** `ctest -R "Gui_tests_run"`
- **Per wave merge:** `ctest -R "Gui_tests_run|FwFeatureTreeWidget_Tests_run"` + `tools/fw-string-leak-grep.sh`
- **Phase gate:** Full suite green + leak-grep clean + SPIKE_LIVE_CHECKLIST obligations recorded before `/gsd-verify-work`.

### Wave 0 Gaps
- [ ] `tests/src/Gui/FwFeatureTree.cpp` — GTest: scoping type-name resolution, Tip-move/mustExecute logic, insertObject placement, reorder+undo, DnD gate BLOCK (covers TREE-01 scoping, TREE-02, TREE-04 logic)
- [ ] `tests/src/Gui/FwFeatureTreeWidget.cpp` — QTEST_MAIN offscreen: tree construction, structure (Origin-first/creation-order/nested), delegate label remap, F2 action present (covers TREE-01 structure, F2)
- [ ] `tests/src/Gui/FwTestGuiBootstrap.h` — reuse existing bootstrap (creates Gui::Application then loads PartDesignGui/SketcherGui so Body/feature types + commands resolve) [VERIFIED present]
- [ ] CMake wiring: add both files to `tests/src/Gui/CMakeLists.txt` (GTest source list + `setup_qt_test`)

*Note: this environment has no configured build tree (Phase 1/2 precedent) — tests are authored compile-intended and CI-green; live items (3D feel, insertion-line cursor) go to a SPIKE_LIVE_CHECKLIST per the established verification-deferral pattern.*

## Spike Plan

> Mirrors Phase 2's native-vs-SARibbon spike discipline: prove the engine before the full build; record the verdict in a `SPIKE.md`. Two spikes, time-boxed.

### Spike A — D-03 Tree reuse (BLOCKS TREE-01)
**Question:** Can `Gui::TreeWidget` (via a thin `FwFeatureTree` subclass) be (a) scoped to the active Body, (b) shown with Origin/planes on top, (c) restyled SW-like — all WITHOUT editing `Tree.cpp` bodies?
**Time-box:** 1 working session (mirror Phase 2's 6-item native spike).
**PASS checklist (all must hold):**
- [ ] FwFeatureTree subclass constructs and mounts in `Fw_FeatureManager` (`TreeWidget(name, parent)` ctor is public — already confirmed).
- [ ] Tree can be scoped to the active Body's subtree from the FreeWorks layer using only Gui signals + `"PartDesign::Body"` type-name literal (NO PartDesign/Sketcher link/include).
- [ ] Origin node renders first with 3 planes + 3 axes + point (already free via `ViewProviderOriginGroupExtension::constructChildren` — confirm it survives scoping).
- [ ] Features render in creation order; sketches nested under their feature (free via `claimChildren` — confirm).
- [ ] Front/Top/Right plane label remap works via `QStyledItemDelegate` (display text only; object Label unchanged).
- [ ] Below-tip greying works via delegate/`QPalette`/flags — NO `Visibility`/property mutation.
- [ ] `tools/fw-string-leak-grep.sh` clean (only `Gui::SolidWorksNavigationStyle` allow-listed).
**FAIL branch:** If any structural override needs non-virtual `DocumentItem`/`DocumentObjectItem` surgery (Pitfall 2), fall back to the heavier-but-additive subclass (override more `TreeWidget` virtuals) — still reusing DnD/rename/transaction machinery; from-scratch is last resort only if even that fails.
**Verdict token:** record `reuse committed` (PASS) or `subclass-fallback committed` (FAIL) in `SPIKE.md`, mirroring Phase 2's `native committed`.

### Spike B — D-04 Body.Tip rollback path (CONFIRMS TREE-02; resolves the questions.md "Body.Tip suppress-below" blocker)
**Question:** Does the `Body.Tip` + recompute path back the SW rollback-bar UX from the Gui layer with only transactioned Tip mutation, and can FreeWorks fire it without linking PartDesign?
**Time-box:** 0.5 session (mostly already CONFIRMED by this research's source reads).
**Status from research — CONFIRMED (HIGH):**
- ✅ `Body.Tip` is a persisted `App::PropertyLink` (BodyBase.h:54).
- ✅ Setting Tip + recompute = suppress-below: `mustExecute()` returns 1 on `Tip.isTouched()` (Body.cpp:94-100).
- ✅ The exact set-tip sequence exists in `CmdPartDesignMoveTip::activated` (CommandBody.cpp:730-744): `openCommand` → `Tip = <feature>` / `Tip = None` → `updateActive`.
- ✅ Insert-at-bar exists: `Body::insertObject(feature, target, after)` (Body.cpp:275-321), reroutes BaseFeature (setBaseProperty, Body.cpp:323-338).
- ✅ 3D suppress-below render exists: `ViewProviderBody::updateData` on Tip change + `showTip`/`DisplayModeBody` (ViewProviderBody.cpp:307-332, 602-641).
- ✅ Only-solid-feature Tip constraint + skip datums/sketches (Body.cpp:176-194; CommandBody.cpp:712-721) — handled by snapping to `getPrevSolidFeature`.
**Open PASS items for the spike to confirm live (deferred to SPIKE_LIVE_CHECKLIST):**
- [ ] Firing via `PartDesign_MoveTip` command-ID (with target feature selected) OR Python `obj.Tip=` `doCommand` string sets Tip from FreeWorks with NO module link (A3).
- [ ] Dragging the bar down / "Roll to End" restores forward Tip (reversibility, Pitfall 4).
- [ ] Live: 3D shows the rolled-back shape; greyed rows; feels like SW (SC5, parity-user track).
**Verdict:** Body.Tip path is the committed engine (D-04 confirmed). No new persisted property (D-05 honored).

## Security Domain

> `security_enforcement: true`, ASVS level 1. This phase is a Gui-layer tree widget — no auth, no network, no secrets, no file parsing of untrusted input. Most ASVS categories do not apply.

### Applicable ASVS Categories
| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | — (no auth surface) |
| V3 Session Management | no | — |
| V4 Access Control | no | — |
| V5 Input Validation | partial | F2 rename / label input → already validated by `Property::setValue`; tree DnD targets validated by `canDropObjectEx`/`canDragObjectToTarget` (prevents invalid model state) |
| V6 Cryptography | no | — never hand-roll; n/a |

### Known Threat Patterns for FreeWorks Gui tree
| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Invalid reorder corrupts model / dependency cycle | Tampering | BLOCK via `canDragObjectToTarget`/`canDropObjectEx`; commit only valid reorders in one transaction (D-11/D-12) [VERIFIED gates exist] |
| Rollback diverges from document (parallel Gui state) | Tampering | `Body.Tip` is single source of truth; no new persisted Gui state (D-05) |
| Module-link leak (FreeWorks → PartDesignGui) breaks merge isolation | Tampering (of fork discipline) | Reference by command-ID/type-name string literals + Gui signals only (Phase 2 precedent); CI `fw-string-leak-grep.sh` + module-link guard |
| Greying via Visibility mutation pollutes `.FCStd`/undo | Tampering | Gui-only greying (delegate/palette/flags); never write properties for presentation (D-06, Pitfall 3) |
| Property write during recompute | Tampering | All mutations via `openTransaction`/`commitTransaction`, never during recompute (CLAUDE.md anti-pattern) |

(Carry forward the Phase 2 threat-closure discipline; the new threats are model-integrity + fork-isolation, both mitigated by reusing existing gates and the string-literal reference rule.)

## Sources

### Primary (HIGH confidence — direct source read this session)
- `src/Mod/Part/App/BodyBase.h:54` — `App::PropertyLink Tip` (rollback property)
- `src/Mod/PartDesign/App/Body.{h,cpp}` — `mustExecute` on `Tip.isTouched` (94-100), `insertObject` (275-321), `setBaseProperty` (323-338), `isSolidFeature` (176-194), `getPrev/NextSolidFeature` (102-158), addObject Tip.setValue (248)
- `src/Mod/PartDesign/Gui/CommandBody.cpp:657-745` — `CmdPartDesignMoveTip` set-tip sequence
- `src/Mod/PartDesign/Gui/ViewProviderBody.{h,cpp}` — Tip tree icon + 3D display (updateData 307-332, show 602-641), DnD overrides (canDragObjectToTarget 593-600, dropObject 549-591)
- `src/Mod/PartDesign/Gui/ViewProviderSketchBased.cpp:77-86` — sketch-under-feature claimChildren
- `src/Gui/ViewProviderOriginGroupExtension.cpp:57-90` — Origin-forced-first
- `src/Gui/Tree.{h,cpp}` — TreeWidget surface (Tree.h:60-341), sortDroppedObjects (3260-3308), F2 rename (607-614, 6536-6556), drawRow (3310), TreeDockWidget (4303), item text (4568)
- `src/Gui/ViewProvider.h:420-493` — claimChildren + DnD gate declarations
- `src/App/Datums.{h,cpp}` — PlaneRoles/AxisRoles/PointRoles (Datums.h:202-206), plane labels (Datums.cpp:270-272)
- `src/Gui/Application.h:130,142,154` — `signalActiveDocument`/`signalActivatedObject`/`signalInEdit`
- `src/Gui/FreeWorks/FwLayout.cpp:101,125` — Fw_FeatureManager placeholder mount point
- `tests/src/Gui/{FwRibbon.cpp,FwRibbonWidget.cpp,CMakeLists.txt}` — headless GTest + QTEST_MAIN patterns

### Secondary (MEDIUM confidence — version-pinned official SW Help via search snippets)
- SOLIDWORKS Help "Rollback Bar" (2013/2014/2016/2025) — drag bar up suppresses features below; add/edit features in rolled-back state; "Roll to End" via drag or right-click [help.solidworks.com/2025/.../c_rollback_bar.htm]
- SOLIDWORKS Help "Reordering Features" (2013–2025) — drag up/down; dragged-over item highlights; dropped feature lands below highlighted item [help.solidworks.com/2025/.../c_reordering_features_dragging.htm]
- SOLIDWORKS Forums — "Cannot reorder. Change would put child feature before parent feature" → SW BLOCKS invalid reorders (matches D-11) [forum.solidworks.com/thread/24847; cati.com parent-child blog]

### Tertiary (LOW confidence — training knowledge, flagged for verification)
- SW Front=XY / Top=XZ / Right=YZ plane correspondence (A1) — verify in parity-user check.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — every reused class confirmed by direct read at the canonical seams.
- Architecture (reuse-and-mount): HIGH — tree structure is already emitted by ViewProviders; rollback engine confirmed present and recompute-aware.
- Pitfalls: HIGH — the App/Gui-separation scoping pitfall and the non-virtual item-construction surface are confirmed from the headers; the spike scopes the residual risk.
- SW parity bar: MEDIUM — confirmed across pinned Help versions via search; direct page fetch blocked (403), so wording is from snippets not full pages.

**Research date:** 2026-06-14
**Valid until:** 2026-07-14 (FreeCAD main is a moving target — re-verify file:line seams if the fork rebases; the API shapes are stable, line numbers may drift).

## RESEARCH COMPLETE
