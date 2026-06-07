# Walking Skeleton — FreeWorks (FreeCAD SolidWorks-Style UI)

**Phase:** 1
**Generated:** 2026-06-06

## Capability Proven End-to-End

> One sentence: the smallest user-visible capability that exercises the full stack.

A SolidWorks user launches FreeCAD, selects **"FreeWorks"** in the workbench selector, and the application mounts a coherent SolidWorks-style dock shell (labeled `Fw_*` placeholder docks in the SW positions, top toolbar area reserved for the future ribbon) while the viewport navigates with SolidWorks mouse conventions — proving the additive `src/Gui/FreeWorks/` module compiles, links, registers, activates, and applies the navigation default end-to-end before any later phase mounts the real ribbon / tree / PropertyManager onto it.

## Architectural Decisions

| Decision | Choice | Rationale |
|---|---|---|
| Module placement | Additive Gui submodule `src/Gui/FreeWorks/`, namespace `FreeWorksGui`, `Fw` class prefix (D-02) | App/Gui separation is a project hard invariant; entire fork UI lives under `src/Gui/`, zero App-layer changes. Additive module is the mergeability backbone. |
| Activation seam | `FwWorkbench : Gui::StdWorkbench` (label "FreeWorks", D-01); `setupDockWindows()` + `activated()` → `FwLayout::install()` | Rides the stable `Gui::Workbench` plugin path; installs the shell with **zero** `MainWindow.cpp` startup edits (Pitfall 1). Subclass `StdWorkbench` (not bare `Workbench`) to inherit the Std command set for the future ribbon. |
| Coexistence model (OQ-1) | Switchable `FwWorkbench` alongside stock FreeCAD; optional "activate FreeWorks at startup" preference, **default OFF** in Phase 1 | Merge-safe default; "boot into FreeWorks" becomes a one-flag product toggle, not an architectural fork. **Confirm at plan review.** |
| Shell contents (OQ-2) | Permanent `Fw_*` dock **names** (`Fw_FeatureManager`, `Fw_PropertyManager` left; `Fw_TaskPane` reserved right) backed by **labeled placeholder widgets**; top toolbar area reserved for the P2 ribbon | Later phases swap *content* into stable dock names without re-layout; a labeled-placeholder shell makes "coherent SW layout" demonstrable now (SHELL-01) without borrowing FreeCAD chrome that must later be unwound. **Confirm at plan review.** |
| Navigation (NAV-01) | Default the `NavigationStyle` preference to the existing upstream `Gui::SolidWorksNavigationStyle` (set only when key unset) | Already shipped + registered upstream; NAV-01 is a preference default, not new nav code (Anti-Pattern 2). |
| macOS nav substitute (OQ-3) | Default macOS to a **modifier-emulated-MMB profile** (preserves SW button semantics); expose `Gui::GestureNavigationStyle` as a one-click trackpad alternative; **flag a Mac-hardware spike** | macOS lacks a default 3-button mouse and `SolidWorksNavigationStyle` has no built-in MMB emulation (verified). Both paths are Gui-only. **Confirm emulation modifier at plan review; spike on real Mac.** |
| Build/CI | Native Qt6 Widgets only (no new deps); one `add_subdirectory(FreeWorks)  # SW-FORK HOOK` in `src/Gui/CMakeLists.txt`; reuse existing per-OS CI matrix | Cross-platform by construction (SHELL-02); existing `sub_buildWindows/Ubuntu/Pixi.yml` compile the new subdirectory for free. |
| Merge discipline | `// SW-FORK HOOK` marker on every unavoidable shared-file edit (exactly two expected: the CMake `add_subdirectory`, and the nav-default preference write) | Greppable for the upstream-sync drill (SC5); keeps the fork mergeable against a moving `main`. |
| Asset/legal | Workbench icon = recreated original art with an `ASSET_PROVENANCE.md` row before it lands; CI provenance guard + "SolidWorks"-string leak grep | No verbatim SW proprietary assets; D-03 trademark-avoidance enforced in CI. |

## Stack Touched in Phase 1

- [x] Project scaffold — new `src/Gui/FreeWorks/` module (`CMakeLists.txt`, `PreCompiled.{h,cpp}`, export macro) compiling + linking into `FreeCADGui`
- [x] Routing equivalent — workbench registration (`Gui.addWorkbench` / `GetClassName → FreeWorksGui::FwWorkbench`); "FreeWorks" appears in the selector and activates
- [x] One real read AND one real write of persisted state — read the `NavigationStyle` preference key; write the SolidWorks default when unset (the skeleton's persisted-state round-trip)
- [x] One real UI interaction wired through the stack — activating the workbench mounts the `Fw_*` placeholder docks via `FwLayout::install()`; the viewport navigates SW-style
- [x] "Deployment"/full-stack run — documented local run: build FreeCAD with FreeWorks linked, launch, select FreeWorks, confirm shell + nav; cross-platform proven by CI build matrix + manual tri-OS checklist

## Out of Scope (Deferred to Later Slices)

> Anything that is *not* in the skeleton. Explicit, to prevent future phases re-litigating Phase 1's minimalism.

- The actual **CommandManager ribbon** (Phase 2) — Phase 1 only reserves the top toolbar area.
- The **FeatureManager design tree**, rollback bar, F2 rename, drag-reorder (Phase 3) — Phase 1 ships an empty `Fw_FeatureManager` placeholder dock.
- The **PropertyManager** slide-in panel, selection-reference boxes, live preview (Phase 4) — Phase 1 ships an empty `Fw_PropertyManager` placeholder dock.
- **Selection parity / on-canvas accelerators** — box/cross select, hover highlight, heads-up toolbar, view cube, S-key, magnifier (Phase 5).
- **Mouse gestures + Instant3D handles** (Phase 6).
- **Full visual theme, Task Pane content, IP legal sign-off** (Phase 7) — Phase 1 ships a minimal `FwTheme` hook + a single workbench icon and reserves `Fw_TaskPane`.
- **Auto-activate-at-startup default ON** — kept a confirmable product toggle, default OFF (OQ-1).

## Subsequent Slice Plan

Each later phase adds one vertical slice on top of this skeleton without altering its architectural decisions:

- **Phase 2 (CommandManager Ribbon):** mount a tabbed ribbon into the reserved top toolbar area, driven by the Std command registry inherited from `StdWorkbench`.
- **Phase 3 (FeatureManager Tree):** swap real tree content into the `Fw_FeatureManager` dock; add rollback bar / F2 / drag-reorder.
- **Phase 4 (PropertyManager):** swap a slide-in command panel into the `Fw_PropertyManager` dock; host `Control`/`TaskView`.
- **Phase 5 (Selection & On-Canvas):** add selection semantics + on-canvas overlays onto the viewport.
- **Phase 6 (Gestures & Instant3D):** add RMB radial gesture + drag handles.
- **Phase 7 (Theme, Task Pane, Legal):** flesh out `FwTheme` into the full QSS/SVG look-alike pack, fill `Fw_TaskPane`, clear the IP checkpoint.

## ⚠ User-Story Format Note (surface at plan review)

The ROADMAP.md Phase 1 `**Goal:**` line is **outcome-shaped but not in canonical "As a / I want to / so that" form**. Per the MVP user-story rules, the planner derived this PLAN-level user story from the existing outcome goal:

> **As a** SolidWorks user, **I want to** launch FreeCAD into a registrable "FreeWorks mode" that mounts a coherent SolidWorks-style dock shell and navigates with SolidWorks mouse conventions cross-platform, **so that** I can start working in a familiar SolidWorks-feeling environment with zero relearning — on a mergeable, asset-clean foundation.

If you want the ROADMAP `**Goal:**` line itself rewritten into canonical user-story form, run `/gsd mvp-phase 1` before executing. The plans below proceed using the derived story.
