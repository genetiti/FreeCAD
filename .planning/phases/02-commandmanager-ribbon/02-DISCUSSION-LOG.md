# Phase 2: CommandManager Ribbon - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-06-07
**Phase:** 2-CommandManager Ribbon
**Areas discussed:** Ribbon engine, Tabs & command source, Contextual switching, Chrome & button look

---

## Ribbon Engine

| Option | Description | Selected |
|--------|-------------|----------|
| Native-first, spike to confirm | Spike native Qt ribbon; ship native if parity met, else SARibbon | ✓ |
| Native, commit now | Skip spike, commit to native, push through rough edges | |
| SARibbon from the start | Vendor SARibbon submodule immediately | |

**User's choice:** Native-first, spike to confirm

| Option | Description | Selected |
|--------|-------------|----------|
| Auto-fall back to SARibbon | Pre-bless SARibbon so planning proceeds without a second decision | ✓ |
| Stop and re-decide | Pause and bring spike findings back before committing | |
| N/A — engine already chosen | (only if a non-spike engine was chosen) | |

**User's choice:** Auto-fall back to SARibbon

| Option | Description | Selected |
|--------|-------------|----------|
| Core ribbon mechanics | Tabs, panels, large labeled icons, flyout split-buttons, context switch; defer galleries/app-button/collapse | ✓ |
| Full SW fidelity | Must also nail galleries, app button, collapse-to-tab | |
| You decide at spike review | Bring screenshots + recommendation, judge at plan review | |

**User's choice:** Core ribbon mechanics
**Notes:** Galleries, application button, and collapse-to-tab/minimize explicitly excluded from the spike pass/fail bar (deferred polish).

---

## Tabs & Command Source

| Option | Description | Selected |
|--------|-------------|----------|
| Curated SW-faithful layout | Hand-authored tab→panel→command-ID map over the existing registry | ✓ |
| Auto-derive from workbench | Tabs built automatically from existing ToolBarItem groups | |
| Hybrid: curated + fallback | Curated core, auto-derive elsewhere | (folded into chosen behavior) |

**User's choice:** Curated SW-faithful layout (with auto-derive fallback for uncurated workbenches, per follow-up below — effectively hybrid)

| Option | Description | Selected |
|--------|-------------|----------|
| Core modeling loop | PartDesign + Sketcher tabs (Features, Sketch, Evaluate, maybe Surfaces) | ✓ |
| Broad parity | Full SW roster across more workbenches now | |
| Single Features tab | One or two tabs as vertical slice | |

**User's choice:** Core modeling loop

| Option | Description | Selected |
|--------|-------------|----------|
| Auto-derive fallback | Uncurated workbenches build tabs from their ToolBarItem groups | ✓ |
| Generic single tab | One plain "Commands" tab dumping that workbench's toolbars | |
| Keep FreeCAD chrome there | Ribbon only for curated workbenches; others use FreeCAD chrome | |

**User's choice:** Auto-derive fallback

| Option | Description | Selected |
|--------|-------------|----------|
| Omit missing, keep extras off | Skip SW commands with no FreeCAD equivalent; don't add FreeCAD-only extras | ✓ |
| Placeholder disabled buttons | Grey-out missing SW commands for visual match | |
| Omit missing, append extras | Skip missing, append useful FreeCAD-only commands | |

**User's choice:** Omit missing, keep extras off
**Notes:** Gap policy governs curated maps; auto-derived tabs naturally show whatever the workbench exposes.

---

## Contextual Switching

| Option | Description | Selected |
|--------|-------------|----------|
| Sketch-edit drives it (v1) | Enter sketch → Sketch tab via Control edit-state; restore on exit | ✓ |
| Edit + selection driven | Also switch on selection type | |
| Full SW contextual tabs | Reproduce SW's full contextual-tab model | |

**User's choice:** Sketch-edit drives it (v1)

| Option | Description | Selected |
|--------|-------------|----------|
| Context wins, then restore | Entering sketch always jumps to Sketch tab; restore prior on exit | ✓ |
| Respect manual choice | Don't auto-switch away from a manually picked tab | |
| You decide | Match SW's documented behavior, verify against SW Help | |

**User's choice:** Context wins, then restore

---

## Chrome & Button Look

| Option | Description | Selected |
|--------|-------------|----------|
| Hide toolbars, keep menu bar | Hide redundant toolbars, keep menu bar as safety net | |
| Hide both, full ribbon-only | Hide menu bar AND toolbars for pure SW look | ✓ |
| Ribbon alongside existing | Add ribbon on top, keep FreeCAD chrome visible | |

**User's choice:** Hide both, full ribbon-only
**Notes:** Surfaces a discoverability risk — captured as D-12 (escape hatch required: keep shortcuts working / command-search / overflow). FreeWorks-scoped and reversible.

| Option | Description | Selected |
|--------|-------------|----------|
| Big labeled icons + flyouts | Large icon-over-label + working flyout split-buttons; theming → Phase 7 | ✓ |
| Structure only, theme later | Functionally correct, defer sizing to Phase 7 | |
| Full visual fidelity now | Nail SW sizing/spacing/accent this phase | |

**User's choice:** Big labeled icons + flyouts

---

## Claude's Discretion

- Spike time-box duration + concrete parity checklist for the core-mechanics bar.
- Curated-map file format and exact tab/panel/command-ID contents (verify vs version-pinned SW Help for the SW2024/2025 default roster).
- Mechanism for reading `Control` edit-state (signal/observer vs polling) and the precise sketch enter/leave hook.
- The exact discoverability escape-hatch mechanism (D-12): shortcuts-only vs command-search vs overflow menu.
- Mount seam: `WorkbenchManipulator` vs direct `FwWorkbench`/`FwLayout` install — pick the most additive.

## Deferred Ideas

- Broader SW contextual tabs (selection/context-driven) beyond sketch-edit.
- Ribbon galleries, application button, collapse-to-tab/minimize.
- User customization of the ribbon (CommandManager customize dialog).
- Full pixel-level SW theming of ribbon buttons (Phase 7).
- Curated rosters for non-core workbenches (Sheet Metal, Weldments, assembly-equivalents).
