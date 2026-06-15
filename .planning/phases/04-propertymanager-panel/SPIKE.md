# SPIKE — D-03 Container Re-host (Phase 04, Plan 04-01 Task 2)

**Decision gate for D-01/D-02/D-03.** Resolves whether the Phase-4 PropertyManager is
built by **re-hosting** FreeCAD's existing `Gui::Control` / `Gui::TaskView::TaskView`
task-panel machinery in the LEFT slot (`reuse-and-rehost committed`) or by a thin
`FwPropertyManager` container that **adopts** the running `TaskView`/`TaskDialog`
(`thin-adopt committed`), and resolves the spike-gated unknowns A1/A2/A3/A4 — each with a
DECISIVE verdict token — by source-read, before any production mount is written.

> **Reference-CAD naming note (D-03):** this is a planning document. It refers to the
> reference CAD product by name in prose rationale only. No new *source-file* identifier
> or string contains that name; the source leak-grep (`tools/fw-string-leak-grep.sh`)
> covers `src/`, not `.planning/`.

---

## Approval basis (READ FIRST — this is a documentation approval)

This gate is a `checkpoint:human-verify`, `gate="blocking"`. The honest disposition,
matching the Phase 1/2/3 precedent for blocking human-verify checkpoints (decisions
`[01-03]`/`[01-04]`/`[02-01]`/Phase-3 spike: *"approved by the user; verification record is
the checklist artifact + CI jobs, not fabricated build/launch results"*):

- **There is no FreeCAD build tree, GUI binary, or CI runner in this environment.** The
  live placement round-trip, the edit-time workbench-switch observation, AND the
  overlays-survive-the-edit-time-WB-switch observation therefore **cannot be executed
  here**.
- The live demonstrations are **deferred** to a signed-off checklist:
  **`src/Gui/FreeWorks/SPIKE_LIVE_CHECKLIST.md`** (`## Phase 4` section appended by this
  plan).
- Each item below is marked with its **evidence class**:
  - **[CODE]** — demonstrated by the committed Task-1 artifacts (the three Wave 0 test
    files wired into CMake). Real, in-repo evidence.
  - **[RESEARCH]** — the seam is confirmed by source-read (`04-RESEARCH.md`, file:line
    citations re-walked in this spike). Every load-bearing claim cites read source.
  - **[LIVE-DEFERRED]** — the on-hardware observation is deferred to
    `SPIKE_LIVE_CHECKLIST.md`. Values shown for such items are **design/expected**, not
    observed-on-hardware, and are labelled as such.

The PASS marks below are granted on the **[CODE]+[RESEARCH]** basis under documentation
approval. The live round-trips remain open checklist obligations, not claimed observations.

---

## A1 — the LEFT-PLACEMENT MECHANISM (the load-bearing item)

**Verified ground truth (R2-F1):** `DockWindowManager::addDockWindow("Tasks", taskView,
Left)` will **NOT move an already-docked Tasks panel.** `addDockWindow()` returns the
existing dock immediately when `widget->parentWidget()` is already a `QDockWidget`
(`DockWindowManager.cpp:256-258`), **before** the `mw->addDockWidget(pos, dw)` placement
(`:278`) and the container-objectName set (`:290`) — both of which run ONLY on the NEW-dock
branch. Stock/PartDesign workbenches dock the Tasks panel **right** via `Std_TaskView`
(`Workbench.cpp:925`, `Qt::RightDockWidgetArea`), so coming from such a workbench
`Control().taskPanel()` is non-null inside an existing right `QDockWidget` that
`addDockWindow` would never move. A "host the registered widget and the container becomes
Tasks" note that ignores this can't-move-an-existing-dock fact is therefore NOT a valid
resolution.

**Resolution — the two-branch placement.** The LEFT placement resolves the host from
`Gui::Control().taskPanel()` (`Control.cpp:55-61`, which calls `getDockWindow("Tasks")`),
walks **up** to its parent `QDockWidget`, and branches on
`getMainWindow()->dockWidgetArea(dock)`:

1. **came-from-right-dock case** — an existing non-left container is re-docked left via
   `Gui::getMainWindow()->addDockWidget(Qt::LeftDockWidgetArea, dock)`. This re-docks the
   EXISTING `QDockWidget` (the only mechanism that moves an already-docked panel, since
   `addDockWindow` early-returns for it).
2. **never-docked case (F1)** — when `taskPanel()` has no parent `QDockWidget`, create the
   dock left via `addDockWindow("Tasks", taskView, Left)`. This is the real case in
   FreeWorks mode: `FwWorkbench::setupDockWindows()` (`FwWorkbench.cpp:95-124`) returns
   ONLY `Fw_FeatureManager`/`Fw_PropertyManager`/`Fw_TaskPane` — never `Std_TaskView` — so
   the framework builds no live Tasks dock until this mount runs.

Idempotency: re-activation re-checks `dockWidgetArea(dock)` and only moves if not already
left.

**The 4-name disambiguation (recorded):**

| Name | What it is | Source |
|------|-----------|--------|
| `"Std_TaskView"` | the **registry KEY** the TaskView is registered under | `MainWindow.cpp:628` |
| toggle-action data | the show/hide action's data carrying the registry key | DockWindowManager toggle wiring |
| `"Tasks"` (container) | the dock-**CONTAINER** objectName (set from the widget objectName on the create path) | `DockWindowManager.cpp:530-535` (`setup()`), `:290` |
| `"Tasks"` (widget) | the child-**WIDGET** objectName on the `TaskView` itself | `MainWindow.cpp:623` |

**Parent-independent lookup (recorded):** `getDockWindow("Tasks")` matches the **container
objectName** (`DockWindowManager.cpp:321-331`) and is **parent-independent** — re-docking
the container left does NOT change its objectName, so `Control().taskPanel() != nullptr`
is preserved across the move. The managed PropertyManager dock identity is therefore
**`"Tasks"`** (R2-F3), and the `Fw_PropertyManager` placeholder is **consumed** (removed
post-setup) so there is ONE left surface, not two.

**Evidence:** [RESEARCH] (`DockWindowManager.cpp:256-258/278/290/321-331/530-535`,
`Control.cpp:55-61`, `MainWindow.cpp:623/628`, `Workbench.cpp:925`,
`FwWorkbench.cpp:95-124`) + [CODE] (`FwPropertyManagerWidget.cpp` asserts the registered
Tasks widget is resolvable offscreen, the baseline 04-02 extends). The live left-move
round-trip is [LIVE-DEFERRED].

> **A1 VERDICT — `left-placement: two-branch (re-dock-existing-left | create-left)`.**
> The two-branch mechanism (re-dock an existing non-left container left via
> `getMainWindow()->addDockWidget(Left, dock)` / create-left via `addDockWindow` only when
> never-docked — R2-F1), the 4-name disambiguation, the parent-independent
> `getDockWindow("Tasks")` lookup, and the `"Tasks"` managed identity (R2-F3, placeholder
> consumed) are all recorded. **PASS.**

---

## A2 — the DOCK-PLACEMENT teardown policy (sub-case of A4)

**Verified ground truth (R2-F2 + R4-BLOCKER):** the deactivated-before-activated-before-
showDialog ordering —

- `Application::activateWorkbench()` calls `oldWb->deactivated()` (`Application.cpp:1984`)
  **BEFORE** `newWb->activated()` (`:2006`);
- PartDesign `ViewProvider::setEdit()` calls `assureWorkbench("PartDesignWorkbench")`
  (`ViewProvider.cpp:165`) **BEFORE** `Control().showDialog()` (`:175`);
- `signalInEdit` fires only after `startEditing()` (`Document.cpp:680-692`).

So on a tree double-click, FreeWorks `deactivated()` runs while `Control().activeDialog()`
is **still null** AND before `signalInEdit` fires. Consequences:

- a teardown guard on `activeDialog() != nullptr` is **TOO LATE** (still null at that
  instant), and
- a **pre-checked edit-in-progress flag is ALSO too late** (still false, because
  `signalInEdit` — which would set it — has not fired — R4-BLOCKER).

**Resolution.** The `"Tasks"` dock is a MainWindow-level `QDockWidget`; leaving it LEFT is
harmless under stock workbenches and survives the `assureWorkbench` round-trip so the edit
lands left regardless of active workbench. Therefore `unmountPropertyManager()` must
**NOT reverse the left placement on `deactivated()` at all** (no synchronous right-move).
`deactivated()` may SCHEDULE a deferred teardown but never moves the dock back right
synchronously. The `signalInEdit` reveal consumer (fired after `startEditing`, within the
same synchronous turn) **re-asserts** the idempotent left placement AND **cancels** the
deferred teardown when the edit lands.

**Evidence:** [RESEARCH] (`Application.cpp:1984/2006`, `ViewProvider.cpp:165/175`,
`Document.cpp:680-692`). Live WB-switch dock observation is [LIVE-DEFERRED].

> **A2 VERDICT — `dock-stays-left: no synchronous right-move on deactivated()`.**
> The dock stays left on `deactivated()`; the deferred teardown is cancelled and the left
> placement re-asserted on `signalInEdit`; the `activeDialog()` guard AND a pre-checked
> flag are both recorded as insufficient per the ordering (R2-F2/R4-BLOCKER). **PASS.**

---

## A4 — the LIFECYCLE / SURVIVABILITY gate (the R3-ROOT / R4-BLOCKER keystone)

A2 fixed the dock *placement*; A4 fixes the *behavior mounted on it*. The SAME ordering
means an edit-time workbench switch (PartDesign `assureWorkbench` / Sketcher edit) fires
FreeWorks `deactivated()` mid-edit.

**Verified ground truth:**

- `FwWorkbench::deactivated()` (`FwWorkbench.cpp:66/71`) calls `restoreStockChrome()` +
  `FwLayout::unmountRibbon()`, which does `s_ribbonContext.reset()` **synchronously**
  (`FwLayout.cpp:280-286`), and — as the 04-02/04-03 plans were written — would also call
  `unmountPropertyManager()` releasing the header / key-filter / pink styler;
- editing a feature TRANSIENTLY activates another workbench DURING `setEdit()` BEFORE
  `showDialog()`: PartDesign `assureWorkbench("PartDesignWorkbench")`
  (`ViewProvider.cpp:165` before `:175`); Sketcher edit → `SketcherWorkbench`
  (`ViewProviderSketch.cpp:635`, activated `:3963`/`:4001`);
- `activateWorkbench()` runs `oldWb->deactivated()` (`Application.cpp:1984`) BEFORE
  `newWb->activated()` (`:2006`); `signalResetEdit`/`signalInEdit` fire LATER
  (`Document.cpp:680-692`, `747-750`) — but WITHIN the same synchronous `activateWorkbench`
  call stack (one event-loop turn).
- **R4-BLOCKER VERIFIED:** because `signalInEdit` fires AFTER `deactivated()` within that
  one turn, a pre-checked edit-in-progress flag (set on `signalInEdit`) is STILL FALSE when
  `deactivated()`→`unmountPropertyManager()` runs — so a flag-guarded teardown would STILL
  strip the chrome.

**Consequences (R3-BLOCKER1 / R3-BLOCKER2 / R6-MAJOR1):**

- the FLOW-01 sketch-exit handler hung off the `s_ribbonContext` `signalResetEdit` consumer
  (`FwRibbonContext.cpp:111-121`) is DESTROYED before sketch exit fires (R3-BLOCKER1 — the
  isolation test passes while the product fails);
- the Features-tab-ready half left SOLELY on the transient `FwRibbonContext`
  `decideOnReset→RestorePrevious` (`FwRibbonContext.cpp:117-119/154-163`) is torn down with
  `s_ribbonContext`, so a real sketch exit can pass the selection tests while FAILING the
  required tab-ready half (R6-MAJOR1);
- a tree double-click lands in a left RAW Tasks dock WITHOUT the FreeWorks chrome
  (R3-BLOCKER2 — fails PROP-01/PROP-02/D-10 in the exact round-trip the plans claim to fix).
  Preserving dock PLACEMENT (the A2 fix) is NOT enough — the BEHAVIOR mounted on it is gone
  too.

**Resolution — the DEFERRED, CANCELLABLE teardown.** `deactivated()`→
`unmountPropertyManager()` does **not** tear down inline and does **not** rely on a
pre-checked flag (too late, R4-BLOCKER) — it **SCHEDULES** the true-exit teardown via a
cancellable `QTimer::singleShot(0, …)` (or an equivalent owned/restartable `QTimer`) and
remembers the pending-teardown handle. Because the WHOLE transient sequence
(`assureWorkbench → deactivated() → activated() → setEdit() → tryStartEditing() → emit
signalInEdit`) runs **synchronously within ONE event-loop turn**, the queued teardown does
NOT fire until control returns to the event loop — by which time `signalInEdit` (and the
FreeWorks re-`activated()` that re-mounts) have already **CANCELLED** it. So the chrome +
**BOTH FLOW-01 halves** (the persistent sketch selection AND the Features-tab-ready landing
— R6-MAJOR1) SURVIVE. For a GENUINE no-edit workbench switch, no `signalInEdit` fires, so
the uncancelled queued teardown runs on the next turn and releases the chrome normally.

The `s_propertyReveal` / FLOW-01 handler (04-04, BOTH halves on the survivable consumer),
the styler/key-filter (04-03), AND the dock placement (A2) all ride this single
deferred-cancellable teardown — none torn down synchronously in `deactivated()`.

**CROSS-PHASE OBSERVATION (do NOT edit shipped phases):** the Phase 2-3 ribbon
(`s_ribbonContext`, `FwLayout.cpp:280-286`) uses the SAME synchronous-teardown-on-
deactivated pattern, so it LIKELY vanishes during edits too — a pre-existing latent issue
flagged here for a **follow-up**, never patched in this plan. The live checklist confirms
the synchronous-within-one-turn assumption the singleShot cancellation relies on.

**Evidence:** [RESEARCH] (`FwWorkbench.cpp:66/71`, `FwLayout.cpp:280-286`,
`FwRibbonContext.cpp:111-121/117-119/154-163`, `ViewProvider.cpp:165/175`,
`ViewProviderSketch.cpp:635/3963/4001`, `Application.cpp:1984/2006`,
`Document.cpp:680-692/747-750`). The overlays-survive observation (chrome + BOTH FLOW-01
halves) and the synchronous-within-one-turn confirmation are [LIVE-DEFERRED].

> **A4 VERDICT — `overlays-survive: deferred-cancellable teardown (QTimer::singleShot(0)
> cancelled by signalInEdit / re-activation)`.** `deactivated()` SCHEDULES a cancellable
> singleShot teardown that `signalInEdit` / re-activation cancels within the same
> event-loop turn, so the chrome + BOTH FLOW-01 halves survive the edit-time WB switch; a
> genuine no-edit switch lets the uncancelled teardown run normally. The Phase 2-3 ribbon
> same-pattern risk is recorded as a cross-phase follow-up observation, not patched here.
> The named mechanism is what Plans 04-02/04-03/04-04 implement. **PASS.**

---

## A3 — the active-reference-box hook + a FreeWorks-owned pink palette ROLE

Resolved against the REAL hosted PartDesign 2-field panel, not a synthetic state machine.

**(a) The active-box hook (R2-F4 / R6-MAJOR2).** NO public active-field hook exists:
`PartDesignGui::ReferenceSelection` is only a `Gui::SelectionFilterGate`
(`ReferenceSelection.h:42-57`), and the real two-field active state is **private** —
`TaskPatternParameters::activeDirectionWidget` (`TaskPatternParameters.h:91`), set at
`TaskPatternParameters.cpp:294-307`, with two real direction widgets
`parametersWidget`/`parametersWidget2` (`.h:88-89`). So a focus+selection-only styler can
pass a synthetic two-field QTEST and still color the WRONG real box.

Disposition: **FOCUS-INFERENCE-FIRST by DEFAULT** (no shared-file edit) — focus +
`Gui::Selection` inference VALIDATED against the real `TaskPatternParameters` linear-pattern
panel (two direction widgets). An **EXPLICIT CONDITIONAL ESCALATION** to a minimal
`// SW-FORK HOOK` const active-field accessor on
`src/Mod/PartDesign/Gui/TaskPatternParameters.{h,cpp}` (returning the private
`activeDirectionWidget`) is taken **ONLY if the spike proves focus inference insufficient**
— a planned, gated, marker-bearing edit that adds those two shared files to 04-03's
`files_modified` and requires developer approval / re-scope (mirroring this plan's D-02
shared-file escalation). So NEITHER branch is ever an UNPLANNED shared-file edit.

**Chosen branch (recorded):** **focus-inference-first (DEFAULT) — no shared-file edit.**
Rationale: the source-read confirms a focus-armed reference box plus the `Gui::Selection`
gate state is sufficient to identify the single armed box for a *visual restyle only*
(FreeWorks does not own fill/highlight/auto-expand — that stays with the hosted panel's
`SelectionFilterGate`/`SelectionObserver`, D-05). **Implied files: NONE shared** (only the
FreeWorks-side `FwReferenceBoxStyler.{h,cpp}` in 04-03). The gated `// SW-FORK HOOK`
accessor on `TaskPatternParameters.{h,cpp}` remains the recorded conditional escalation,
to be promoted into 04-03's `files_modified` (with the marker + developer approval) ONLY if
a live run proves focus inference colors the wrong real box.

**(b) The pink SOURCE — a functional QPalette role (R2-F6).** `QPalette::Highlight` is the
SYSTEM highlight (typically blue, NOT pink — `FwFeatureTreeDelegate.cpp:154-159` reads
Disabled/Text + Highlight), and `FwTheme::apply()` is CURRENTLY a no-op
(`FwTheme.cpp:31-39`), so a bare `activeReferenceBoxTone()→QColor` accessor while
`apply()` stays no-op does NOT satisfy D-06's "via a QPalette role" (D-06 locked,
"functional state now"). Disposition: `FwTheme::apply()` becomes **FUNCTIONAL** — install
the single FreeWorks-owned pink VALUE into a chosen `QPalette` ROLE on the reference-box
widget's **LOCAL** palette (QPalette has no custom roles, so the pink is carried on the
box widget's local palette via a chosen role, reversible), and the styler READS
`widget->palette().color(<role>)`. The pink VALUE lives once in `FwTheme`, expressed
WITHOUT a hex string literal (a `QColor(r,g,b)` int-RGB constructor or a palette-derived/
HSV-shifted value — never `QColor("#…")`), so D-06's "never a hex literal" holds at the
source site and the 04-03 hex-grep covers `FwTheme.{h,cpp}`.

**Named role (recorded):** **`QPalette::Midlight`** is the carrier role for the
FreeWorks-owned pink VALUE on the reference-box widget's local palette. It is a standard
role not used as the reserved prompt-icon blue (`QPalette::Link`) and not the system
selection blue (`QPalette::Highlight`), so the active pink is unambiguous and the read
mechanism (`widget->palette().color(QPalette::Midlight)`) is exercised by the Wave 0
`FwReferenceBoxStyler.cpp` test. Plan 04-03's `FwTheme::apply()` populates this role.

**Evidence:** [RESEARCH] (`ReferenceSelection.h:42-57`, `TaskPatternParameters.h:88-91`,
`TaskPatternParameters.cpp:294-307`, `FwTheme.cpp:31-39`,
`FwFeatureTreeDelegate.cpp:154-159`) + [CODE] (`FwReferenceBoxStyler.cpp` asserts the
read-the-role contract on a local palette). The real-panel color-the-correct-box
observation is [LIVE-DEFERRED].

> **A3 VERDICT — `active-box: focus-inference-first (DEFAULT, no shared file) | gated
> SW-FORK HOOK escalation; pink via QPalette::Midlight role`.** The active-box hook is
> focus-inference-first by default (validated against the real `TaskPatternParameters`
> 2-field panel — R6-MAJOR2), with the gated `// SW-FORK HOOK` accessor on
> `TaskPatternParameters.{h,cpp}` (+ marker + 04-03 `files_modified` entry + developer
> approval) as the conditional escalation only if focus inference is proven insufficient;
> the chosen branch (focus-inference-first, no shared file) and implied files (none shared)
> are recorded. `FwTheme::apply()` becomes functional, populating `QPalette::Midlight` on
> the box widget's local palette; the styler reads `widget->palette().color(Midlight)`
> (R2-F6). **PASS.**

---

## Reuse-primitive confirmation

[RESEARCH]+[CODE] — the reuse primitives are present unmodified:

- `Gui::TaskView::TaskGroup`/`TaskBox` collapsible rollouts (`TaskView.h:67/82`);
- `Gui::Control().accept()`/`reject()` slots (`Control.h:98-100`) wired to the
  `TaskEditControl` `QDialogButtonBox` (`TaskView.cpp:644-686`);
- `TaskView::keyPressEvent` Enter→accept / Esc→reject incl. the macOS workaround
  (`TaskView.cpp:393-457`);
- the non-modal nature of the TaskView dock (a `QDockWidget`, never `QDialog::exec`).

So NO new task-panel backend is built (D-01/D-04/D-07).

---

## Verdict

**reuse-and-rehost committed.**

A1 (two-branch left placement — R2-F1) is source-resolved with the parent-independent
`getDockWindow("Tasks")` lookup and the `"Tasks"` managed identity preserved; A2 (dock
stays left on `deactivated()`, no synchronous right-move — R2-F2/R4-BLOCKER) holds; A4
(overlays-survive via the deferred-cancellable teardown — R3-ROOT/R4-BLOCKER/R6-MAJOR1)
names the singleShot policy that survives the chrome + BOTH FLOW-01 halves; A3 (active-box
focus-inference-first by default + functional `FwTheme` `QPalette::Midlight` role —
R2-F4/R2-F6/R6-MAJOR2) is resolved with the chosen branch + implied files recorded. No
item FAILed.

**Re-hosting the existing `"Tasks"` `TaskView` in the LEFT slot (re-dock existing /
create-left) is the committed engine for Plans 04-02, 04-03, and 04-04.** No second
decision round; the `thin-adopt` fallback (a thin `FwPropertyManager` that adopts the
running `TaskView`/`TaskDialog`) is **not** triggered and remains shelved unless the
deferred live checklist later reveals a genuine re-host gap. Plans 04-02/04-03/04-04 MUST
implement the **deferred-cancellable survivable-lifecycle policy** (A4) AND the A3
active-box branch (focus-inference-first; gated `// SW-FORK HOOK` escalation only if proven
insufficient) — they must NOT proceed on the placement/lifecycle/active-box strategy merely
because 04-01 ran.

### Open obligation (carried, not blocking)

The live left-placement round-trip (A1), the edit-time WB-switch dock observation (A2), the
**overlays-survive observation** (A4 — header / key filter / styler AND the FLOW-01 reset
handler BOTH halves still firing through the edit-time WB switch — R6-MAJOR1), and the
**synchronous-within-one-turn confirmation** (the basis the singleShot cancellation relies
on) are deferred to **`src/Gui/FreeWorks/SPIKE_LIVE_CHECKLIST.md` (`## Phase 4`)**, to be
signed off when a build tree is available — mirroring the Phase 1/2/3 deferral pattern.
Should a live run contradict the design (e.g. the chrome cannot survive the edit-time WB
switch, or focus inference colors the wrong real box), the recorded fallbacks are the A4
escalate path (`overlays-lost`) and the A3 gated `// SW-FORK HOOK` accessor, respectively.

### thin-adopt fallback reference (NOT triggered — recorded for completeness)

If a later live finding flips A1 (neither re-docking an existing Tasks container left nor
creating one left preserves `getDockWindow("Tasks")` resolution), the FAIL fallback is the
thin-adopt path: an `FwPropertyManager` QWidget registered under `Fw_PropertyManager` that,
on `Control().showDialog`, adopts the running `TaskView` into the left dock for the edit's
lifetime and releases it on close — reusing the `TaskDialog`/`accept`/`reject` machinery,
never rebuilding it. A one-line `// SW-FORK HOOK` in `Control.cpp`/`TaskView.cpp` is the
final escalation and must be flagged for developer approval. (The A3 active-field
`// SW-FORK HOOK` accessor is a separate, narrower, conditional hook on
`TaskPatternParameters.{h,cpp}`, also developer-flagged.)

---

## Sign-off

- **D-03 container re-host decision:** reuse-and-rehost committed.
- **A1:** `left-placement: two-branch (re-dock-existing-left | create-left)` — PASS.
- **A2:** `dock-stays-left: no synchronous right-move on deactivated()` — PASS.
- **A4:** `overlays-survive: deferred-cancellable teardown (QTimer::singleShot(0) cancelled
  by signalInEdit / re-activation)` — PASS.
- **A3:** `active-box: focus-inference-first (DEFAULT, no shared file) | gated SW-FORK HOOK
  escalation; pink via QPalette::Midlight role` — PASS.
- **Approval mode:** documentation approval (user-authorized), Phase 1-3 precedent
  `[01-03]`/`[01-04]`/`[02-01]`; no build tree / GUI / CI in this environment.
- **Live verification record:** `src/Gui/FreeWorks/SPIKE_LIVE_CHECKLIST.md` (`## Phase 4`,
  open).
- **Resume signal:** `approved: reuse-and-rehost` → Plans 04-02/04-03/04-04 build on the
  place-the-Tasks-TaskView-left engine with the deferred-cancellable survivable-lifecycle
  policy and the focus-inference-first active-box branch. (`approved: thin-adopt` to commit
  the fallback instead.) The live placement round-trip, the WB-switch dock observation, the
  A4 overlays-survive observation (BOTH FLOW-01 halves), and the synchronous-within-one-turn
  confirmation remain open obligations in `SPIKE_LIVE_CHECKLIST.md`.
