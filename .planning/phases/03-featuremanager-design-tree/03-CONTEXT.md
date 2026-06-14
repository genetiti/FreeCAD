# Phase 3: FeatureManager Design Tree - Context

**Gathered:** 2026-06-14
**Status:** Ready for planning
**Decision mode:** User delegated all gray-area calls to Claude ("you choose the best options for me"). Decisions below are Claude's recommendations, grounded in the codebase scout, the SolidWorks-parity bar, and the locked Phase 1/2 precedents. They are LOCKED for planning unless the user revisits.

<domain>
## Phase Boundary

Deliver a left-docked, SolidWorks-styled **FeatureManager design tree** for the active PartDesign Body: features in creation order under a top **Origin/planes node**, sketches nested under their consuming feature, plus three interactions — a **draggable rollback bar** (suppress-below + recompute), **F2 rename**, and **drag-to-reorder history with validity feedback**. Requirements: **TREE-01, TREE-02, TREE-04**.

**Not this phase:** double-click-to-edit-in-PropertyManager (TREE-03 → Phase 4, needs the PropertyManager container); SW icon art / exact colors / fonts (→ Phase 7 theming); selection/canvas behavior (→ Phase 5).
</domain>

<decisions>
## Implementation Decisions

### A. Tree foundation strategy (the keystone decision)
- **D-01: Reuse-first — wrap and restyle FreeCAD's existing `Gui::TreeWidget`, re-hosted in the left `Fw_FeatureManager` dock.** This mirrors the Phase 2 precedent (drive the ribbon from the existing command registry, don't duplicate). The stock tree already provides — for free — drag-drop reorder (`TreeWidget::startDrag`/`dropMimeData`/`sortDroppedObjects`), F2 inline rename (`relabelObjectAction` → `Label.setValue`, transaction-wrapped), `claimChildren` nesting, and the SelectionObserver wiring. Re-implementing those is wasted risk.
- **D-02: Achieve the SolidWorks presentation via FreeWorks-side configuration/ViewProvider hooks, NOT by editing `src/Gui/Tree.cpp` bodies.** The SW view differs from the stock tree (single active-Body focus, Origin-on-top, creation order) — produce that by scoping/configuring the widget from the FreeWorks layer. Prefer, in order: (1) a thin `FwFeatureTree : Gui::TreeWidget` subclass that overrides presentation, or (2) FreeWorks ViewProvider/filter configuration — whichever avoids shared-file edits. Any unavoidable shared touch gets `// SW-FORK HOOK`.
- **D-03 (PLAN-TIME SPIKE, mirrors Phase 2's native spike):** Before the full build, confirm the existing `Gui::TreeWidget` can be (a) scoped to the active Body, (b) reordered to show Origin/planes at the top, and (c) restyled SW-like **without editing `Tree.cpp` bodies**. PASS → reuse-and-wrap is the committed engine. FAIL → fall back to a thin `Gui::TreeWidget` subclass (still additive, still reusing its DnD/rename/transaction machinery — NOT a from-scratch tree). A from-scratch tree widget is explicitly the last resort, not the default.

### B. Rollback bar ↔ `Body.Tip`
- **D-04: The rollback bar drives the REAL `Body.Tip` (true suppress-below + recompute), reusing the existing engine — NOT a Gui-only cosmetic marker.** Dragging the bar commits a transaction that sets `Body.Tip` via the same path as `PartDesign_MoveTip` (`CommandBody.cpp:657`, `Tip.setValue`); insert-at-bar reuses `Body::insertObject(feature, target, after)` (`Body.h:84`). A cosmetic-only marker that didn't recompute would "look like, not behave like" SW — failing SC5 (must *feel* like SW) and the core value.
- **D-05: Reconcile SC2's "rollback state held entirely in the Gui layer" as follows:** the rollback **bar widget** (its position, drag affordance, the greying of below-bar rows) is Gui-only; the **model state** it produces uses the existing, already-persisted `Body.Tip` App property (FreeCAD's legitimate, transactional, recompute-aware mechanism — and SW likewise persists rollback state). We add **NO new persisted rollback property** and NO parallel suppression that could diverge from the document — `Tip` is the single source of truth. This honors "no `.FCStd` pollution": we reuse an existing property rather than inventing Gui state that leaks into save files.
- **D-06: Features below the rollback bar are visually de-emphasized (greyed) in the tree (Gui-only styling); the 3D view shows the rolled-back result** via FreeCAD's existing `ViewProviderBody` tip display (`showTip`/`DisplayModeBody`, `ViewProviderBody.cpp:307`). Dragging the bar back down restores (sets Tip forward) — reversible, like SW.

### C. Tree structure fidelity
- **D-07 (in scope):** active-Body-scoped tree; a top **Origin node** containing the 3 reference planes + 3 axes + origin point (FreeCAD's `App::Origin` already groups these); **features in creation order**; **sketches nested under their consuming feature** (reuse `ViewProviderSketchBased::claimChildren`).
- **D-08 (in scope, cheap + high recognition value):** display the origin planes under **SolidWorks display names** ("Front Plane / Top Plane / Right Plane") as a Gui-only label mapping over FreeCAD's XY/XZ/YZ planes — underlying objects unchanged. This is structural/labeling, not icon art.
- **D-09 (deferred):** SW **icon artwork**, exact colors/fonts → Phase 7 theming. Extra SW top-level nodes (Material, Annotations, Sensors, Equations, Mates, Lights/Cameras) → out of scope this milestone unless a later phase needs them. Phase 3 ships Origin + features + sketches only.

### D. Drag-to-reorder validity feedback
- **D-10: Reuse FreeCAD's dependency-aware drag-drop machinery** (`TreeWidget` DnD + the ViewProvider gates `canDragObjectToTarget` / `canDropObjectEx`, `ViewProvider.h:445-525`, with PartDesign's `ViewProviderBody` overrides). It already encodes solid-feature ordering rules.
- **D-11: Show a visible insertion-line affordance during drag; on an invalid reorder (a feature moved before a dependency, or past a dependent), BLOCK the drop** — forbidden/no-drop cursor, no commit. Allow-and-flag-error is REJECTED: it can leave the model in a broken recompute state, violating SC4's "clean undo/redo, no `.FCStd` pollution." This matches SW's safe-reorder feel.
- **D-12: Valid reorders commit through a single document transaction** (`Document::openTransaction`/`commitTransaction`) via `Body::insertObject` / the Group `PropertyLinkList` reorder (`Tree.cpp:3266`) → clean undo/redo.

### E. F2 rename (folded — largely decided by reuse)
- **D-13: Reuse the existing F2 path verbatim** (`relabelObjectAction`, `Key_F2` → `editItem` → `DocumentObjectItem::setData` → `Label.setValue`, transaction-wrapped at `Tree.cpp:6546`). Target the user-facing `Label`. Only work required: ensure it is wired/active in the `Fw_FeatureManager`-hosted tree.

### Claude's Discretion
- Exact widget seam to mount in the dock (subclass vs configured instance) — resolved by the D-03 spike during planning/research.
- Precise greying mechanism for below-bar rows (stylesheet vs `QPalette`/item flags) — but no inline hex (Phase 7 owns color; mirror Phase 2's palette-only rule).
- Whether the rollback bar is a custom drawn row/separator in the tree vs an overlay widget — planner's call, provided it drives `Body.Tip` per D-04.
</decisions>

<canonical_refs>
## Canonical References

**Downstream agents (researcher, planner) MUST read these before planning or implementing.**

### Phase scope & requirements
- `.planning/ROADMAP.md` § "Phase 3: FeatureManager Design Tree" — goal + 5 success criteria (note SC5: a daily-SW-user acceptance check that rollback *feels* like SW)
- `.planning/REQUIREMENTS.md` — TREE-01 (left-docked creation-order tree, origin/planes, nested sketches), TREE-02 (rollback bar + insert/edit mid-history), TREE-04 (drag-reorder with validity feedback). TREE-03 (double-click→PropertyManager + F2) is Phase 4 — only the F2 half is relevant here.
- `.planning/research/questions.md` — flagged plan-time spike: **"Body.Tip suppress-below code path … blocks TREE-02"** (D-03/D-04 address this) and the FeatureManager-interactions doc-research item (blocks TREE-01..04).

### Rollback engine (Body.Tip) — the key seam for TREE-02
- `src/Mod/Part/App/BodyBase.h:54` — `App::PropertyLink Tip;` (the rollback point property)
- `src/Mod/PartDesign/App/Body.h:70,84,89,95,140,146` — `addObject`, `insertObject(feature,target,after)`, `removeObject`, `isAfterInsertPoint`, `getPrevSolidFeature`, `getNextSolidFeature`
- `src/Mod/PartDesign/App/Body.cpp:248,361,96` — `Tip.setValue` on add; Tip auto-repair on remove; `mustExecute()` recompute trigger on `Tip.isTouched()`
- `src/Mod/PartDesign/Gui/CommandBody.cpp:657,671` — `CmdPartDesignMoveTip` / `"PartDesign_MoveTip"` — the rollback bar's direct ancestor; reuse/wrap it
- `src/Mod/PartDesign/Gui/ViewProviderBody.cpp:307,395` — Tip-driven tree icon + `showTip`/`DisplayModeBody` 3D display (the suppress-below visual)

### Tree widget + reuse seams
- `src/Gui/Tree.h:60,348,489,580` — `TreeWidget`, `DocumentItem`, `DocumentObjectItem`, `TreePanel`
- `src/Gui/Tree.cpp:4303` — `TreeDockWidget` (existing dock host pattern to learn from)
- `src/Gui/Tree.cpp:609,614,1438,6536-6551` — F2 rename path (relabel action → editItem → setData → transactioned `Label.setValue`)
- `src/Gui/Tree.cpp:3222,3266,3296` — `sortDroppedObjects` reorder engine (Group `PropertyLinkList::setValue`, treeRank)
- `src/Gui/ViewProvider.h:427,445-460,483,525` — `claimChildren`, `canDragObjects`/`canDragObjectToTarget`/`dragObject`, `canDropObjectEx`/`dropObjectEx` (dependency-aware DnD gates)
- `src/Mod/PartDesign/Gui/ViewProviderBody.h:91-96` + `ViewProviderBody.cpp:549` — Body's drag/drop overrides
- `src/Mod/PartDesign/Gui/ViewProviderSketchBased.h:49` (+ Hole/Loft) — `claimChildren` (sketch-under-feature nesting, reuse for D-07)

### Phase 1 seam (mount target) & fork discipline
- `src/Gui/FreeWorks/FwLayout.cpp:100-101,121-127` — `ensureDock(... "Fw_FeatureManager" ...)` — currently a `QLabel` placeholder; Phase 3 replaces its content
- `.planning/phases/01-solidworks-mode-foundation/01-01-SUMMARY.md` — Fw_* dock seam, `target_sources` build pattern, single `// SW-FORK HOOK`, observe-the-DOM
- `.planning/phases/01-solidworks-mode-foundation/01-CONTEXT.md` — `Fw` naming (namespace `FreeWorksGui`, dir `src/Gui/FreeWorks/`), no-"SolidWorks"-identifier rule, additive-module discipline
- `.planning/phases/02-commandmanager-ribbon/02-SECURITY.md` + `02-01-SUMMARY.md` — established fork patterns to mirror (headless GTest bootstrap, leak-grep, App/Gui separation, ParameterGrp persistence)

### Architecture & conventions (anti-patterns to honor)
- `CLAUDE.md` + `.planning/codebase/ARCHITECTURE.md` — App/Gui separation; **do NOT modify the document during recompute**; mutate via Property setters + transactions only; Coin3D ref/unref discipline
- `.planning/codebase/CONVENTIONS.md`, `.planning/codebase/STRUCTURE.md`, `.planning/codebase/TESTING.md` — C++20/Qt6.8 naming, where Gui code lives, headless GTest/ctest patterns (mirror Phase 1/2)
</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets (reuse, don't rebuild — Phase 2 precedent)
- **`Gui::TreeWidget`** (`src/Gui/Tree.*`): full DnD reorder, F2 rename, transactions, `claimChildren` nesting, SelectionObserver — the foundation per D-01.
- **`Body.Tip` + `PartDesign_MoveTip`**: ready-made rollback engine (set-tip → recompute) per D-04; `Body::insertObject` for insert-at-bar.
- **`App::Origin`**: already groups the 3 planes + 3 axes + point for the D-07 Origin node.
- **`ViewProvider` DnD gates**: dependency-aware drag/drop validation for D-10/D-11.

### Established Patterns (constrain this phase)
- Observe-the-DOM: the tree reflects the live `App::Document` via SelectionObserver/ViewProvider signals; never caches a divergent model.
- All App-state mutations (Tip move, reorder, rename) go through `Document::openTransaction`/`commitTransaction` — clean undo/redo, no `.FCStd` pollution.
- FreeWorks code is additive (`target_sources(FreeCADGui PRIVATE ...)`), namespace `FreeWorksGui`, dir `src/Gui/FreeWorks/`, no "SolidWorks" identifier in source, `// SW-FORK HOOK` for any unavoidable shared touch.

### Integration Points
- Mount: replace the `Fw_FeatureManager` placeholder in `FwLayout.cpp` with the (re)hosted tree.
- Rollback/recompute: through `Body.Tip` (App) — the only persisted state touched.
- Reorder/rename: through existing transactioned Property setters.
</code_context>

<specifics>
## Specific Ideas
- The rollback bar should *feel* like SolidWorks (SC5 is a daily-SW-user acceptance check) — reversible drag, real suppress-below + recompute, greyed below-bar rows. "Looks like" is not enough.
- Plane display names should read "Front/Top/Right Plane" (D-08) for instant SW recognition.
- The Phase-3 spike (D-03) is the analog of Phase 2's native-vs-SARibbon spike: prove reuse-and-wrap works before committing the full build; document the verdict in a SPIKE.md.
</specifics>

<deferred>
## Deferred Ideas
- **Double-click feature → edit in PropertyManager (TREE-03):** Phase 4 — needs the PropertyManager container.
- **SW icon artwork, colors, fonts:** Phase 7 (Visual Theme).
- **Extra SW tree nodes** (Material, Annotations, Sensors, Equations, Mates, Lights/Cameras): out of scope this milestone.
- **Multi-body / assembly tree semantics:** out of scope (single active Body this phase).
- **Folder/grouping features inside the tree (SW folders):** not in TREE-01/02/04 — defer unless a later phase requires it.
</deferred>

---

*Phase: 03-featuremanager-design-tree*
*Context gathered: 2026-06-14 (user-delegated decisions, Claude-recommended)*
