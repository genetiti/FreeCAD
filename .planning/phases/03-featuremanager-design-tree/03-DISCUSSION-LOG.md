# Phase 3: FeatureManager Design Tree - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-06-14
**Phase:** 3-featuremanager-design-tree
**Areas presented:** Tree foundation strategy, Rollback bar ↔ Body.Tip, Tree structure fidelity, Drag-reorder validity feedback
**Discussion mode:** User selected "You choose the best options for me" — delegated all four gray-area calls to Claude. Decisions below are Claude's recommendations, grounded in the codebase scout, the SolidWorks-parity bar, and the Phase 1/2 precedents.

---

## A. Tree foundation strategy

| Option | Description | Selected |
|--------|-------------|----------|
| Reuse + restyle `Gui::TreeWidget` | Re-host the existing tree (inherits DnD, F2, transactions, claimChildren) in the left Fw dock | ✓ (Claude) |
| Build a new SW-faithful Fw tree | Tighter control of structure/feel, but re-implements DnD/rename/rollback wiring | |

**Choice:** Reuse-first (D-01/D-02), with a plan-time spike (D-03) to confirm SW presentation is achievable without editing `Tree.cpp` bodies; thin `Gui::TreeWidget` subclass is the fallback, from-scratch is last resort.
**Rationale:** Matches the Phase 2 precedent (reuse FreeCAD infrastructure, don't duplicate). The stock tree already provides every interaction SC1/SC3/SC4 need. Most merge-safe.

---

## B. Rollback bar ↔ Body.Tip

| Option | Description | Selected |
|--------|-------------|----------|
| Drive the real `Body.Tip` | True suppress-below + recompute via the existing `PartDesign_MoveTip`/`Tip.setValue` engine | ✓ (Claude) |
| Gui-only cosmetic marker | A non-recomputing visual marker that never touches the document | |

**Choice:** Drive the real `Body.Tip` (D-04), reconciling SC2's "Gui-layer" wording as bar-widget-is-Gui / model-state-uses-existing-Tip with no new persisted property (D-05); below-bar rows greyed, 3D shows rolled-back result (D-06).
**Rationale:** SC5 demands it *feels* like SW; a cosmetic-only marker would "look like, not behave like." `Body.Tip` is FreeCAD's legitimate, persisted, transactional, recompute-aware mechanism — and SW also persists rollback state.

---

## C. Tree structure fidelity

| Option | Description | Selected |
|--------|-------------|----------|
| SW structure now, SW look later | Origin/planes node + nested sketches + SW plane names this phase; icons/colors → Phase 7 | ✓ (Claude) |
| Full SW fidelity now | Also icon art, extra SW nodes (Material/Annotations/…) | |

**Choice:** Structure in scope (D-07: active-Body, Origin node with 3 planes/axes/point, creation order, sketches nested via claimChildren; D-08: SW plane display names Front/Top/Right). Icon art/colors deferred to Phase 7; extra SW top-level nodes out of scope (D-09).
**Rationale:** Keeps the phase a focused structural slice; theming is Phase 7's job. SW plane names are cheap, high-recognition, and structural (not art).

---

## D. Drag-reorder validity feedback

| Option | Description | Selected |
|--------|-------------|----------|
| Block invalid drops (dependency-aware) | Insertion line + forbidden cursor + no commit on invalid reorder; valid → single transaction | ✓ (Claude) |
| Allow-and-flag-error | Permit any reorder, mark resulting recompute errors | |

**Choice:** Reuse FreeCAD's dependency-aware DnD gates (D-10); insertion-line affordance + BLOCK invalid reorders (D-11); valid reorders commit through one transaction via `Body::insertObject`/Group reorder (D-12).
**Rationale:** Allow-and-flag can leave the model in a broken recompute state, violating SC4's "clean undo/redo, no `.FCStd` pollution." Blocking matches SW's safe-reorder feel and reuses gates FreeCAD already enforces.

---

## E. F2 rename (folded — largely decided by reuse)

**Choice:** Reuse the existing F2 path verbatim (D-13): `relabelObjectAction` → `editItem` → transactioned `Label.setValue`. Only work: ensure it's wired in the Fw-hosted tree. Not presented as a standalone gray area because reuse settles it.

## Claude's Discretion
- Widget seam (subclass vs configured instance) — resolved by the D-03 spike.
- Below-bar greying mechanism (palette/item-flags; no inline hex — Phase 7 owns color).
- Rollback bar as drawn tree row vs overlay widget — planner's call, provided it drives `Body.Tip`.

## Deferred Ideas
- TREE-03 double-click→PropertyManager → Phase 4.
- SW icon art / colors / fonts → Phase 7.
- Extra SW tree nodes (Material, Annotations, Sensors, Equations, Mates) → out of scope this milestone.
- Multi-body / assembly tree, SW folders → out of scope this phase.
