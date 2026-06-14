# Phase 4: PropertyManager Panel - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-06-14
**Phase:** 04-propertymanager-panel
**Areas presented:** Container / hosting strategy, Pink selection-reference box (PROP-02), Slide-in + non-modal feel (PROP-01), Sketch→feature handoff (FLOW-01)

---

## Decision mode

The user was presented the four gray areas (multiSelect) and chose **"You choose the best options"** — delegating all gray-area calls to Claude, exactly as in Phase 3. No single area was deep-dived interactively; Claude made each call grounded in the codebase scout, the SolidWorks-parity bar, and the locked Phase 1/2/3 precedents, and locked them in CONTEXT.md (D-01 … D-12).

---

## Container / hosting strategy (the keystone)

| Option | Description | Selected |
|--------|-------------|----------|
| Re-host existing `Gui::Control`/`TaskView` left + restyle | Reuse-first; `showDialog`/`taskPanel()` redirected into `Fw_PropertyManager`; D-03 spike to confirm no `Control.cpp`/`TaskView.cpp` body edits | ✓ (Claude) |
| New `FwPropertyManager` container embedding running `TaskDialog` widgets | Thin container that adopts the live panels (the D-03 FAIL fallback) | fallback |
| From-scratch task-panel system | Rejected — last resort only | |

**Claude's choice:** Re-host `Control`/`TaskView` (D-01/D-02), gated by the D-03 plan-time spike (mirrors Phase 2 native-ribbon / Phase 3 tree-reuse spikes).
**Notes:** Hosting `Control` left also lands TREE-03 edit panels left for free (D-09) and keeps existing PartDesign/Sketcher panels working unmodified (SC4).

---

## Pink selection-reference box (PROP-02)

| Option | Description | Selected |
|--------|-------------|----------|
| Thin FreeWorks reference widget driving existing `Gui::Selection` | Click→pink-active, pick fills/highlights/auto-expands, clear/remove; no new selection backend | ✓ (Claude) |
| Restyle the reference widgets existing task panels already use | Less control over the SW pink-active feel | |

**Claude's choice:** Thin FreeWorks affordance over `Gui::Selection` (D-05); pink-when-active treated as a FUNCTIONAL state color allowed this phase, final hue/art Phase 7 (D-06).
**Notes:** Pink = active box, blue = prompt icons (official Help). Reuses the Phase-3 `FwSelectionGuard` selection pattern; Phase 5 still owns selection *behavior*.

---

## Slide-in + non-modal feel (PROP-01)

| Option | Description | Selected |
|--------|-------------|----------|
| Animated slide if low-risk, else instant reveal; non-modal locked | MVP must-have = panel appears left-docked on command start; slide is a refinement | ✓ (Claude) |
| Mandatory heavyweight slide animation | Risk of cross-platform polish blocking the functional panel | |

**Claude's choice:** Non-modal/semi-modal locked (D-07); slide is a light affordance, animate only if cheap/cross-platform-clean (D-08), slide FEEL → live checklist.
**Notes:** Fully-modal is explicitly rejected in REQUIREMENTS (would break selection-box picking).

---

## Sketch→feature handoff (FLOW-01)

| Option | Description | Selected |
|--------|-------------|----------|
| On sketch exit: auto-select sketch as profile + switch to Features tab, user clicks the feature | True SW behavior; reuses `signalResetEdit` + Phase-2 ribbon context | ✓ (Claude) |
| Auto-start a specific feature command on sketch exit | Guesses user intent — rejected | |

**Claude's choice:** Auto-select profile + Features-tab handoff, no auto-launch (D-11); plane/face→Sketch reuses existing sketch-creation flow (D-12).

---

## Claude's Discretion

- Exact container seam (re-host live TaskView vs. thin adopting container) — resolved by the D-03 spike.
- Slide animation vs. instant reveal (animate only if low-risk).
- Precise active-box pink tone + ✓/✗ glyphs (functional/palette now, art Phase 7).
- Key-event override mechanism for the SW Enter/Escape/Tab semantics (D-10).

## Deferred Ideas

- SW icon artwork / exact final colors / fonts → Phase 7 (Visual Theme).
- Selection-model / box-select / canvas accelerators / context toolbars → Phase 5.
- Multi-body / assembly PropertyManager semantics → out of scope this milestone.
- Bespoke SW-exact per-command panel layouts beyond existing TaskDialogs → future refinement.
