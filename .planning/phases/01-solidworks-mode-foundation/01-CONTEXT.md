# Phase 1: SolidWorks Mode Foundation - Context

**Gathered:** 2026-06-06
**Status:** Ready for planning

<domain>
## Phase Boundary

Deliver a registrable, merge-safe "SolidWorks mode" shell for FreeCAD that a SolidWorks user can launch into, that navigates with SolidWorks mouse conventions, and that builds/runs on Windows, macOS, and Linux — built as an additive module with upstream-merge and asset-provenance discipline so all later UI phases mount onto it without fighting upstream.

**In scope:** SHELL-01 (SolidWorks-mode activation shell), SHELL-02 (cross-platform build/run), NAV-01 (SolidWorks navigation default); plus the non-functional foundation — additive `src/Gui/FreeWorks/` module, `// SW-FORK HOOK` merge discipline, asset-provenance CI guard, headless `.FCStd`-compat gate, upstream-sync drill.

**Out of scope (own phases):** the actual ribbon (Phase 2), FeatureManager tree (Phase 3), PropertyManager (Phase 4), selection/on-canvas accelerators (Phase 5), gestures/Instant3D (Phase 6), visual theme + Task Pane + legal sign-off (Phase 7).
</domain>

<decisions>
## Implementation Decisions

### Product Identity & Naming
- **D-01:** Working product name is **FreeWorks** — a SolidWorks-evocative but legally distinct coinage (Free + "...Works"). Treat as a **working name**; a real trademark search must clear it before public distribution (Phase 7 legal checkpoint, ROADMAP Phase 7 SC4/SC5).
- **D-02:** Code/module naming uses the **`Fw`** prefix, **superseding the research's `Sw`/`src/Gui/SolidWorks/` convention everywhere downstream**:
  - Directory: `src/Gui/FreeWorks/`
  - Classes: `FwWorkbench`, `FwLayout`, `FwTheme`, `FwRibbon`, `FwFeatureManager`, `FwPropertyManager`, etc.
  - Namespace: `FreeWorksGui`
  - The workbench shown in FreeCAD's workbench selector is labeled **"FreeWorks"**.
- **D-03:** The string "SolidWorks" (and any SolidWorks wordmark/logo) must appear in **neither** user-facing surfaces (window title, About box, workbench label, installer, menu/UI strings) **nor** code identifiers/paths. Internal descriptive comments referencing SolidWorks *behavior* are fine; identifiers are not.
  - Note: FreeCAD already ships `Gui::SolidWorksNavigationStyle` (upstream class — do NOT rename it; default to it via preference). The trademark-avoidance rule applies to *new* FreeWorks code, not to referencing the existing upstream class.

### Claude's Discretion — delegated to research/planning, confirm at plan review
The user chose to decide only Product Identity in this discussion. These three Phase-1 gray areas are delegated to the researcher/planner to resolve technically; surface recommendations at plan review for a quick user confirm:
- **Activation & coexistence model:** whether FreeWorks mode is a switchable `FwWorkbench` alongside stock FreeCAD (merge-safe default per research) vs. booting straight into FreeWorks with classic FreeCAD chrome hidden. *Lean:* implement via the `FwWorkbench` activation seam regardless; auto-activate-at-startup is a product toggle to confirm.
- **Phase-1 shell contents:** what the empty shell shows before ribbon/tree/panels exist — labeled placeholder docks establishing the SW dock layout vs. dock skeleton + nav reusing FreeCAD's current tree/combo temporarily.
- **macOS navigation substitute:** SW nav assumes a 3-button mouse (MMB rotate); define the rotate/pan substitute for Mac trackpad / 2-button mice (modifier-emulated MMB vs. trackpad gestures). See ROADMAP Phase 1 SC3 "explicit macOS no-middle-button / trackpad profile."
</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase scope & requirements
- `.planning/ROADMAP.md` § Phase 1 — goal, mode (mvp), 5 success criteria
- `.planning/REQUIREMENTS.md` — SHELL-01, SHELL-02, NAV-01 (exact behavior wording)
- `.planning/PROJECT.md` — product vision, constraints (App/Gui separation, upstream-tracking, no proprietary assets)

### Architecture & integration (the HOW)
- `.planning/research/ARCHITECTURE.md` — **primary** for this phase: additive `src/Gui/SolidWorks/`→(now `FreeWorks/`) module, `FwWorkbench` activation seam, hook points, build order, upstream-mergeability strategy, anti-patterns. **Apply D-02 `Fw` naming over its `Sw` examples.**
- `.planning/research/SUMMARY.md` — cross-cutting findings, suggested build order
- `.planning/research/STACK.md` — Qt6/Coin3D tooling, what's already in FreeCAD vs must-add
- `.planning/codebase/ARCHITECTURE.md` — App/Gui separation, Workbench/ViewProvider, anti-patterns (direct property mutation, modifying doc during recompute, Coin3D ref/unref)
- `.planning/codebase/STRUCTURE.md`, `.planning/codebase/CONVENTIONS.md` — where Gui/module code lives, CMake + InitGui registration patterns
- `.planning/codebase/TESTING.md` — GTest/ctest, how changes get verified (for the CI gates)

### Risk & legal
- `.planning/research/PITFALLS.md` — upstream-merge hell prevention (`// SW-FORK HOOK` discipline, mirror branch, small frequent syncs), asset-provenance CI guard, App/Gui-contract breakage, legal asset line
- `.planning/research/SOLIDWORKS-UI.md` — verified SW navigation defaults (rotate=MMB, pan=Ctrl+MMB, zoom-to-cursor, roll=Alt+MMB, dolly=Shift+MMB, middle-click-entity rotate-about) for NAV-01
- `.planning/research/questions.md` — plan-time open questions (incl. macOS nav profile)
</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets (from research/ARCHITECTURE.md — verified against this checkout)
- **`Gui::SolidWorksNavigationStyle`** (`src/Gui/Navigation/SolidWorksNavigationStyle.cpp`, registered in `SoFCDB.cpp`): NAV-01 is essentially free — default it via the `NavigationStyle` preference key. No new navigation code.
- **`Gui::Workbench`** (`src/Gui/Workbench.{h,cpp}`): subclass as `FwWorkbench`; `activate()` already calls `setupToolBars()`/`setupMenuBar()`/`setupDockWindows()` — the install seam for the FreeWorks layout with no `MainWindow` startup edits.
- **`DockWindowManager::registerDockWindow()`** (`src/Gui/DockWindowManager.h`): register new FreeWorks dock names (e.g. `Fw_FeatureManager`) to avoid touching the churned `Std_TreeView`/`Std_ComboView` blocks in `MainWindow.cpp`.
- **`ToolBarAreaWidget` / `ToolBarArea::TopToolBarArea`** (`src/Gui/ToolBarAreaWidget.h`): where the ribbon mounts later — Phase 1 only needs the layout shell.
- **`BitmapFactory`** + Qt QSS / `PreferencePackManager`, `StyleParameters/`: theming substrate for `FwTheme`.

### Established Patterns (must follow)
- **Additive module + Workbench activation** is the mergeability backbone: keep ALL FreeWorks code under `src/Gui/FreeWorks/`; the only shared-file edits should be one entry in `src/Gui/CMakeLists.txt` and (at most) a nav-style preference default — mark any unavoidable shared-file touch with `// SW-FORK HOOK`.
- **Observe-the-DOM, never mutate directly**: FreeWorks widgets read via signals/getters, write only via `Gui::Command` / property setters / `Control().accept()`. Honors codebase ARCHITECTURE.md anti-patterns.
- **No App-layer changes** — the entire module lives under `src/Gui/`.

### Integration Points
- `src/Gui/CMakeLists.txt` — register the new `FreeWorks/` subdirectory (the primary unavoidable shared edit).
- `NavigationStyle` preference key — set `Gui::SolidWorksNavigationStyle` as the FreeWorks default.
- Workbench registration (module `InitGui.py` / C++ `Gui::addWorkbench` path) — register `FwWorkbench` like any module workbench.
</code_context>

<specifics>
## Specific Ideas

- Product name verbatim: **FreeWorks**. Code prefix verbatim: **`Fw`**; namespace **`FreeWorksGui`**; directory **`src/Gui/FreeWorks/`**.
- Apply the `Fw` naming as a find/replace over the research's `Sw*` examples — they are otherwise correct.
</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope. (The three undiscussed Phase-1 gray areas are not deferred to other phases; they are recorded under "Claude's Discretion" above for research/planning to resolve within this phase.)
</deferred>

---

*Phase: 1-SolidWorks Mode Foundation*
*Context gathered: 2026-06-06*
