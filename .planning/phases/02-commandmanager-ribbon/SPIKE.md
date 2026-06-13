# SPIKE — Native Qt Ribbon vs SARibbon (Phase 02, Plan 02-01 Task 3)

**Decision gate for D-01/D-02/D-03/D-04.** Resolves whether the CommandManager ribbon
is built natively on Qt 6.8 (`QTabWidget` + `QToolBar`) or falls back to the pre-blessed
SARibbon submodule, by walking the 6-item D-03 parity checklist from
`02-RESEARCH.md § "Spike Plan (D-02/D-03/D-04)"`.

> **Reference-CAD naming note (D-03):** this is a planning document. It refers to the
> reference CAD product by name in prose rationale only. No new *source-file* identifier
> or string contains that name; the source leak-grep (`tools/fw-string-leak-grep.sh`)
> covers `src/`, not `.planning/`.

---

## Approval basis (READ FIRST — this is a documentation approval)

This gate is a `checkpoint:human-verify`, `gate="blocking"`. The honest disposition,
matching the Phase 1 precedent for blocking human-verify checkpoints (decisions
`[01-03]` / `[01-04]`: *"approved by the user; verification record is the checklist
artifact + CI jobs, not fabricated build/launch results"*):

- **There is no FreeCAD build tree, GUI binary, or CI runner in this environment.** The
  live spike harness (throwaway 3-tab `FwRibbon` mounted in the top area, run against a
  live PartDesign Body + sketch) therefore **cannot be executed here**.
- The user **explicitly approved clearing this gate by documentation** (the Phase 1
  precedent), with the live demonstration **deferred** to a signed-off checklist:
  **`src/Gui/FreeWorks/SPIKE_LIVE_CHECKLIST.md`**.
- Each item below is marked with its **evidence class**:
  - **[CODE]** — demonstrated by the committed Tasks 1-2 artifacts (`FwRibbon.{h,cpp}`,
    the headless `Gui_tests_run` resolution tests, the `FwRibbonWidget` Qt offscreen
    construction test). This is real, in-repo evidence.
  - **[RESEARCH]** — the primitive is confirmed to exist by `02-RESEARCH.md`
    (every D-03 primitive was confirmed present *before* this spike; the spike is
    integration-risk validation, not capability discovery) and converged across **3
    cross-AI review cycles to 0 HIGH concerns**.
  - **[LIVE-DEFERRED]** — the on-hardware visual/interaction observation is deferred to
    `SPIKE_LIVE_CHECKLIST.md`. Values shown for such items are **design/expected**
    values, **not observed-on-hardware results**, and are labelled as such.

The PASS marks below are granted on the **[CODE]+[RESEARCH]** basis under the user's
documentation approval. The live round-trip remains an open checklist obligation, not a
claimed observation.

---

## D-03 parity checklist (6 items)

### Class A — primitives the spike code (Tasks 1-2 `FwRibbon`) directly builds

**Item 1 — Native `QTabWidget` with ≥3 tabs (Features / Sketch / Evaluate).**
`FwRibbon` *is* a `QTabWidget`; `addTabFromCommandIds()` adds one page per call, so a
3-tab build is `addTabFromCommandIds` ×3. The `FwRibbonWidget` Qt test constructs an
`FwRibbon`, calls `addTabFromCommandIds`, and `QCOMPARE`s `tabCount()` under an offscreen
`QApplication`. **[CODE]** — **PASS**.

**Item 2 — Each tab hosts a `QToolBar` panel of large icon-over-label buttons
(32px icon, `Qt::ToolButtonTextUnderIcon`); ≥1 tab shows ≥2 panels with a 1px separator.**
`addTabFromCommandIds()` builds a `QToolBar` panel with
`setToolButtonStyle(Qt::ToolButtonTextUnderIcon)` and `setIconSize(QSize(32,32))`
(`FwRibbon.cpp:65-66`), the UI-SPEC large-icon metric, color from `QPalette`/`QStyle`
only. The multi-panel-per-tab + 1px separator layout is the explicit scope of **Plan
02-02** (this spike builds one panel per tab — minimal but real); the separator is a
stock `QToolBar::addSeparator()` primitive confirmed present. **[CODE]** (single panel,
32px labeled buttons) **+ [RESEARCH]** (multi-panel/separator) — **PASS**.

**Item 3 — Clicking a primary button fires the real command (e.g. `PartDesign_Pad`
opens the PartDesign Pad task).**
The command→button seam is `cmd->addTo(panel)` (`FwRibbon.cpp:81`) — the *same* path
every stock FreeCAD toolbar uses; the resulting `QAction` is the live command trigger, no
duplicated backend. The resolution test proves `PartDesign_Pad` / `PartDesign_Pocket`
resolve to non-null `Gui::Command*` after the module-GUI bootstrap, so `addTo` has a real
action to add. The *visual* confirmation that the Pad task panel opens on click is
**[LIVE-DEFERRED]**; the firing mechanism is **[CODE]**. — **PASS**.

**Item 4 — ≥1 working flyout split-button from a `*_Comp*` group id
(primary fires, MenuButtonPopup dropdown lists alternatives; native, no hand-rolled menu).**
`PartDesign_CompPrimitiveSubtractive` resolves (group-command resolution test passes) and
flows through the identical `cmd->addTo(panel)` path, which for an action group yields a
native `QToolButton` in `MenuButtonPopup` mode via `ActionGroup::addTo()`
(`Action.cpp:472-485`) — confirmed in research, no custom `QMenu` wiring. The dropdown's
on-screen alternative list is **[LIVE-DEFERRED]**; the native split-button generation is
**[CODE]+[RESEARCH]**. — **PASS**.

### Class B — context-switch LIVE restore proof (headline RIBBON-02 behavior)

**Item 5 — Live enter→Sketch-tab→exit→restore round-trip; PASS only if after-tab ==
before-tab.**
The wiring is a throwaway `Gui::Document` `signalInEdit`/`signalResetEdit` subscription
calling `setCurrentIndex(<Sketch tab>)` on enter and `setCurrentIndex(<prior index>)` on
reset — both are stock `QTabWidget` calls and stock `Gui` signals confirmed present in
research; the productionized nested-enter state machine is **Plan 02-04**.

Observed tab indices — **DESIGN/EXPECTED values, to be confirmed on hardware via
`SPIKE_LIVE_CHECKLIST.md` (NOT observed-on-hardware in this environment):**

| Checkpoint | Tab | Index |
|------------|-----------|-------|
| before (active when sketch entered) | Features | **0** |
| in-sketch (after `signalInEdit`) | Sketch | **1** |
| after (after `signalResetEdit`) | Features | **0** |

after (0) == before (0) → restore holds by design. **[RESEARCH] + design-wired; live
demonstration is an open obligation in `SPIKE_LIVE_CHECKLIST.md`.** — **PASS (doc-approved;
live round-trip deferred).**

> Honesty flag: per the gate's strict wording, item 5 wants *observed* indices with
> after == before. Those three indices above are **expected** values from the design, not
> a hardware observation. The user's documentation approval covers this deferral; the live
> enter→switch→exit→restore demonstration must still be recorded against
> `SPIKE_LIVE_CHECKLIST.md` when a build exists, and Plan 02-04 carries the productionized
> behavior with its own verification.

### Class C — subjective look

**Item 6 — Reads "convincingly SW-like" at a glance (visibly bigger than a stock toolbar;
tabbed).**
A 32px icon-over-2-line-label tabbed bar mounted in the top area is, by construction,
visibly larger than FreeCAD's stock ~16-24px toolbars and is tabbed — the two objective
proxies for the subjective judgement. Final glance-test on hardware is **[LIVE-DEFERRED]**.
**[RESEARCH] + design** — **PASS (provisional; confirm on live glance-test).**

---

## Verdict

**native committed.**

All four Class-A primitives are code-demonstrated and/or research-confirmed; the Class-B
context-switch restore is design-wired and holds by construction (after == before); the
Class-C look passes its objective proxies. No Class-A item failed, and the 2-day time-box
was not a factor (the spike is integration validation of already-confirmed primitives).
The native Qt 6.8 engine reaches the D-03 parity bar.

**Native Qt (`FwRibbon` = `QTabWidget` + `QToolBar`) is the committed ribbon engine for
Plans 02-02, 02-03, and 02-04.** No second decision round; the SARibbon fallback (D-04) is
**not** triggered and remains shelved unless the deferred live checklist later reveals a
genuine native gap.

### Open obligation (carried, not blocking)

The live enter→switch→exit→restore demonstration with *observed* tab indices (item 5) and
the on-hardware visual confirmations for items 3, 4, 6 are deferred to
**`src/Gui/FreeWorks/SPIKE_LIVE_CHECKLIST.md`**, to be signed off when a build tree is
available — mirroring Phase 1's `TRIOS_LAUNCH_CHECKLIST.md` deferral pattern. Should the
live run contradict the design (e.g. native cannot restore the prior tab on Qt 6.8), the
SARibbon vendor-vet gate below is the recorded fallback.

### SARibbon fallback reference (NOT triggered — recorded for completeness)

If a later live finding flips this verdict, D-04 fallback steps (UI-SPEC § Registry
Safety): vendor **SARibbon MIT v2.8.0** as a git submodule at **`src/3rdParty/SARibbon`**,
CMake `add_subdirectory` behind a single `// SW-FORK HOOK`; confirm MIT license; pin
v2.8.0; clear `tools/fw-string-leak-grep.sh` + `tools/fw-provenance-guard.sh`; **no Pixi /
Conda dep** (vendored for reproducible tri-OS builds); then re-scope Plans 02-02 / 02-03 /
02-04 actions to the SARibbon API before execution.

---

## Sign-off

- **Native-vs-SARibbon decision:** native committed.
- **Approval mode:** documentation approval (user-authorized), Phase 1 precedent
  `[01-03]`/`[01-04]`; no build tree / GUI / CI in this environment.
- **Live verification record:** `src/Gui/FreeWorks/SPIKE_LIVE_CHECKLIST.md` (open).
- **Resume signal:** `approved: native` → Plans 02-02/02-03/02-04 build on native Qt.
