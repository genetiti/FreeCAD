# Phase 1: SolidWorks Mode Foundation - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-06-06
**Phase:** 1-SolidWorks Mode Foundation
**Areas discussed:** Product identity / name

---

## Gray Area Selection

| Option | Description | Selected |
|--------|-------------|----------|
| Activation & coexistence | Boot-into-SW vs switchable workbench alongside stock FreeCAD | |
| Product identity / name | Distinct fork name threading through title/About/workbench/installer | ✓ |
| Phase-1 shell contents | Placeholder docks vs skeleton + temporary reuse of existing tree | |
| macOS nav substitute | Rotate/pan substitute for the missing middle button | |

**User's choice:** Product identity / name (only)
**Notes:** Other three areas delegated to research/planning (recorded under Claude's Discretion in CONTEXT.md).

---

## Product Identity / Name

### Direction
| Option | Description | Selected |
|--------|-------------|----------|
| Neutral mechanical-CAD name | Clearly-distinct coinage (Datum/Forge/Solidus) — safest legally | |
| SolidWorks-evocative coinage | Familiar but distinct (FreeWorks/SolidFree) — needs trademark check | ✓ |
| FreeCAD sub-brand | Mode/edition name under FreeCAD branding | |
| I'll give the name | User provides | |

**User's choice:** SolidWorks-evocative coinage

### Specific name
| Option | Description | Selected |
|--------|-------------|----------|
| FreeWorks | Free + Works — clean, distinct from full "SolidWorks" mark | ✓ |
| SolidFree | Solid + Free — evokes solid-modeling directly | |
| SolidForge | Solid + Forge — mechanical, distinct | |
| I'll type my own | User provides | |

**User's choice:** **FreeWorks** (working name; trademark search deferred to Phase 7 legal checkpoint)

### Code prefix / module naming
| Option | Description | Selected |
|--------|-------------|----------|
| FreeWorks-branded (Fw) | `src/Gui/FreeWorks/`, `FwWorkbench`, namespace `FreeWorksGui` — keeps trademark out of codebase | ✓ |
| Keep Sw (SolidWorks-mode) | `src/Gui/SolidWorks/`, `SwWorkbench` as research wrote it | |
| Neutral (Sw = "Solid Workbench") | Keep terse `Sw` but document as "Solid Workbench" | |

**User's choice:** FreeWorks-branded (`Fw`) — supersedes the research's `Sw` convention everywhere downstream
**Notes:** "SolidWorks" must not appear in user-facing strings or new code identifiers; the existing upstream `Gui::SolidWorksNavigationStyle` class is referenced (not renamed).

---

## Claude's Discretion

Delegated to research/planning, to confirm at plan review:
- Activation & coexistence model (switchable `FwWorkbench` vs boot-into-FreeWorks)
- Phase-1 shell contents (placeholder docks vs skeleton + temporary tree reuse)
- macOS navigation substitute for the missing middle button

## Deferred Ideas

None — discussion stayed within phase scope.
