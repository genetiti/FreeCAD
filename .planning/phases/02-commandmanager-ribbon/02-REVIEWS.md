---
phase: 2
cycle: 3
reviewers: [codex]
reviewed_at: 2026-06-07T18:40:00Z
plans_reviewed: [02-01-PLAN.md, 02-02-PLAN.md, 02-03-PLAN.md, 02-04-PLAN.md]
prior_cycle_high: 4
current_cycle_high: 0
trajectory: [8, 4, 0]
---

# Cross-AI Plan Review — Phase 2 (Convergence Cycle 3, FINAL)

> Convergence trajectory: cycle 1 = 8 HIGH, cycle 2 = 4 HIGH, cycle 3 = 0 HIGH.
> This cycle (1) confirms whether each of the 4 cycle-2 HIGH concerns is GENUINELY
> resolved in the revised plan text, and (2) surfaces any remaining or newly-introduced
> HIGH concerns. Codex was run against the revised plans, and the orchestrator
> independently re-verified every load-bearing source claim in this checkout (see
> "Orchestrator Verification").

## Codex Review

**Cycle-2 HIGH Statuses**

| Concern | Status | Justification |
|---|---|---|
| 1. Bootstrap did not create `Gui::Application::Instance` | **RESOLVED** | Plan 02-01 now requires `new Gui::Application(false)` before module imports (02-01-PLAN.md:119,124), with ordering + assertion, and an acceptance check that the singleton creation precedes `import PartDesignGui` (02-01-PLAN.md:130). Matches the real guard at `AppPartDesignGui.cpp:104-107` and the canonical setup at `FreeCADGuiPy.cpp:200`. |
| 2. `Std_Measure`/`Std_MassProperties` owner module missing | **RESOLVED** | Plan 02-01 imports `MeasureGui` and `MatGui` (02-01-PLAN.md:119,124) and adds a direct `Std_Measure` resolution assertion (02-01-PLAN.md:133). Plan 02-02 strengthens with per-row Evaluate resolution for `Std_Measure`/`Std_MassProperties`/`Part_CheckGeometry`/`Materials_InspectMaterial` (02-02-PLAN.md:144). Source confirms ownership at `Measure/Gui/Command.cpp:48,94`. |
| 3. Mount tests used generic `QMainWindow`, not `Gui::MainWindow` | **RESOLVED** | Plan 02-03 explicitly requires real `Gui::MainWindow` (02-03-PLAN.md:24,103,110), with acceptance requiring `Gui::MainWindow` + `Gui::getMainWindow()` in the mount test (02-03-PLAN.md:121). Source confirms the singleton is set at `MainWindow.cpp:358`. |
| 4. Spike gate was reachability-only, not live restore | **RESOLVED** | Plan 02-01 requires a live enter-sketch → Sketch-tab → exit → restore demonstration with before/in-sketch/after indices and `after == before` (02-01-PLAN.md:198); the `native committed` verdict is gated on that live restore (02-01-PLAN.md:203), and acceptance rejects a mere "reachable on native" note (02-01-PLAN.md:214). |

**New HIGH Concerns:** None found.

**Non-HIGH Note**
- **MEDIUM:** Plan 02-03's `hideStockChrome()` uses `setState(names, ForceHidden)` after mounting the ribbon (02-03-PLAN.md:110). The wording implies `names` excludes `Fw_RibbonToolBar`, but an explicit acceptance check that the ribbon remains visible after `hideStockChrome()` would harden the gate. (This is the same MEDIUM raised in cycle 2 — already acknowledged in the must-haves; not a regression.)

## Cycle-2 Resolution Table

| # | Cycle-2 HIGH | Status | Justification (plan + verified source) |
|---|---|---|---|
| 1 | Incomplete GUI test bootstrap — never creates `Gui::Application::Instance` | **FULLY RESOLVED** | 02-01 must-have + Task 1 behavior/action specify `if (!Gui::Application::Instance) new Gui::Application(false);` BEFORE module imports, mirroring `FreeCADGuiPy.cpp:200-201`; ordering asserted in acceptance (singleton line < first import line) and `Gui::Application::Instance != nullptr` asserted before imports. Guard verified at `AppPartDesignGui.cpp:104-107`; `Instance = this` verified at `Application.cpp:746`. |
| 2 | Evaluate-tab commands need MeasureGui (+MatGui/PartGui) | **FULLY RESOLVED** | 02-01 bootstrap now imports PartDesignGui (pulls PartGui+SketcherGui), SketcherGui, MeasureGui, MatGui; acceptance greps all four import strings and asserts `getCommandByName("Std_Measure") != nullptr`. 02-02 per-row test fails loudly on ANY unresolved curated row incl. all Evaluate IDs. Ownership verified at `Measure/Gui/Command.cpp:48,94` and `Material/Gui/Command.cpp:143,170`. |
| 3 | Plan 03 mount tests use generic `QMainWindow` | **FULLY RESOLVED** | 02-03 must-have + Task 1 action + acceptance require constructing/using a real `Gui::MainWindow` (its ctor sets `instance = this`), and asserting on `Gui::getMainWindow()` for both mount and inspection so the production `Gui::getMainWindow()->addToolBar(...)` seam is exercised. Verified `instance = this` at `MainWindow.cpp:358`; `getInstance()` at `MainWindow.cpp:538`; inline `getMainWindow()` at `MainWindow.h:398`. |
| 4 | Spike gate reachability-only (cycle-1 #6 remainder) | **FULLY RESOLVED** | 02-01 Task 3 splits the D-03 checklist into evidence classes A/B/C; class-B item 5 now mandates a LIVE enter→switch→exit→restore round-trip with recorded before/in-sketch/after indices and PASS only if `after == before`; `native committed` cannot be written unless item 5 passed; acceptance explicitly states a "reachable on native" note does NOT satisfy item 5. |

**Resolution tally:** 4 of 4 cycle-2 HIGH FULLY RESOLVED. Combined with cycle 2's 6 fully-resolved cycle-1 HIGHs, all 8 original + 4 cycle-2 HIGH concerns are now closed.

## Remaining / New HIGH Concerns (this cycle)

None.

## Orchestrator Verification

The orchestrator independently re-verified every load-bearing source claim the revised plans depend on, in this checkout:

- **GUI singleton guard (concern 1):** CONFIRMED — `src/Mod/PartDesign/Gui/AppPartDesignGui.cpp:104-107` raises `ImportError "Cannot load Gui module in console application."` when `Gui::Application::Instance` is null. (Plan cites "103-105"; actual is 104-107 — a 1-line drift, claim unchanged.)
- **Headless singleton-creation pattern (concern 1):** CONFIRMED — `src/Main/FreeCADGuiPy.cpp:200-201` does `static Gui::Application* app = new Gui::Application(false);` inside `if (!Gui::Application::Instance)`. `Application.cpp:746` sets `Instance = this` in the `Application(bool)` ctor.
- **MeasureGui / MatGui ownership (concern 2):** CONFIRMED — `Std_Measure` (`Command.cpp:48`), `Std_MassProperties` (`Command.cpp:94`) in `src/Mod/Measure/Gui/`; `Materials_InspectMaterial`/`Materials_InspectAppearance` (`Command.cpp:143,170`) in `src/Mod/Material/Gui/`.
- **Gui::MainWindow singleton (concern 3):** CONFIRMED — `src/Gui/MainWindow.cpp:358` `instance = this;`; `getInstance()` at 538; `inline getMainWindow()` at `MainWindow.h:398`. A plain `QMainWindow` would NOT set this — the revised test correctly targets `Gui::MainWindow`.
- **toolBarAreaWidget signature (cycle-1 #3 basis):** CONFIRMED — `ToolBarManager.h` declares `ToolBarAreaWidget* toolBarAreaWidget(QWidget* toolBar) const;` (takes a widget, not an enum) — the cycle-1 fix rationale holds.

## Consensus Summary

Single external reviewer (Codex) this cycle, cross-checked against source by the orchestrator. Both agree: **all 4 cycle-2 HIGH concerns are fully resolved**, and **no new HIGH concerns** were introduced. The convergence trajectory is clean: **8 → 4 → 0 HIGH**. The revisions are genuine mechanism/contract changes (explicit singleton creation + ordered module imports with per-row resolution asserts; real `Gui::MainWindow` mount target; a live enter→switch→exit→restore spike gate), not citation polish — each is backed by a verified source fact in this checkout.

Phase 2 plans are **HIGH-clean** and ready to execute.

### Agreed Strengths
- The GUI test bootstrap is now a complete, ordered sequence (App init → QApplication → `Gui::Application` singleton → owning-module imports) with a loud assertion if the singleton is missing — it can no longer silently assert nothing.
- The per-row Evaluate resolution test (02-02) is a load-bearing guard: it fails if any owning module import is dropped from the bootstrap, preventing silent D-08 degradation.
- Plan 03 tests now exercise the real `Gui::getMainWindow()` production seam via `Gui::MainWindow`, closing the "passes while production is broken" gap.
- The spike gate's class-A/B/C evidence split with a mandatory live restore round-trip makes the native-vs-SARibbon decision an engineering verdict, not subjective theater.

### Agreed Concerns (highest priority)
None at HIGH severity. One residual MEDIUM (harden the `hideStockChrome()` acceptance to assert `Fw_RibbonToolBar` stays visible) is already acknowledged in the 02-03 must-haves.

### Execution-time risks to watch (not blocking plan defects)
- The bootstrap mirrors only the singleton-creation line of `FreeCADGuiPy.setupWithoutGUI()` and omits the subsequent `SoDB::init()`/`SoNodeKit::init()`/`SoInteraction::init()`/`SoFCDB::init()` calls. GUI-module import registration is type-system-only (`PROPERTY_SOURCE`/`init()` register `Base::Type`, not Coin nodes), so this should not block imports — but no existing repo test currently constructs `Gui::Application` or `Gui::MainWindow`, so this is pioneering territory. If an import or `Gui::MainWindow` construction trips on uninitialized Coin/SoFCDB at execution time, add the SoDB/SoFCDB init block from `FreeCADGuiPy.cpp:205-214` to the bootstrap. Flag for execution, not a plan rewrite.

### Divergent Views
None — single reviewer this cycle.

---

## Action Routing

All four cycle-2 HIGH concerns are fully resolved and no new HIGH concerns remain. The
convergence loop (8 → 4 → 0) is complete. Phase 2 plans are ready to execute:

```
/gsd-execute-phase 2
```

Watch the one execution-time risk above (SoDB/SoFCDB init in the bootstrap) during Wave 0,
and optionally harden the `hideStockChrome()` acceptance (the residual MEDIUM) when landing Plan 03.
