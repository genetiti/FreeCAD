---
phase: 02
slug: commandmanager-ribbon
status: verified
threats_open: 0
threats_total: 13
threats_closed: 13
asvs_level: 1
block_on: high
created: 2026-06-13
---

<!-- SPDX-License-Identifier: LGPL-2.1-or-later -->

# SECURITY.md — Phase 02: CommandManager Ribbon

**Audit mode:** Verify declared threat mitigations exist in implemented code (register authored at plan time; no blind vuln scan).
**ASVS Level:** 1 | **block_on:** high
**Implementation files:** READ-ONLY (this audit modified none).
**Phase diff base:** `19d064b283..HEAD`
**Verdict:** SECURED — 13/13 threats CLOSED (mitigations verified present / accepts valid / supply-chain threat confirmed moot).

---

## Threat verification (by disposition)

### MITIGATE — mitigation present in code

| Threat | Category | Verdict | Evidence |
|--------|----------|---------|----------|
| T-02-01 | Repudiation/Legal | CLOSED | `tools/fw-string-leak-grep.sh` exits 0 on `src/Gui/FreeWorks/` (only allow-listed `Gui::SolidWorksNavigationStyle`). Independent grep of phase-02 new source + new test files (`FwRibbon.cpp/h`, `FwRibbonMap.cpp/h`, `FwRibbonContext.cpp/h`, `tests/.../FwRibbon.cpp`, `FwRibbonWidget.cpp`, `FwTestGuiBootstrap.h`) finds zero bare "SolidWorks" tokens. |
| T-02-02 | Tampering | CLOSED | `git diff 19d064b283..HEAD` touches only `src/Gui/FreeWorks/`, `tests/src/Gui/`, `.planning/`. No `src/Gui/MainWindow.cpp` or shared-Gui edit. `src/Gui/FreeWorks/CMakeLists.txt` and `tests/src/Gui/CMakeLists.txt` deltas are append-only source/test-list additions. Only shared hook `src/Gui/CMakeLists.txt:17 add_subdirectory(FreeWorks)  // SW-FORK HOOK` is pre-existing (not in this phase diff) and marked. |
| T-02-03 | Info Disclosure | CLOSED | `grep '#include <Mod/'` in `src/Gui/FreeWorks/` = 0. Only `App/` include is `FwRibbon.cpp:42 <App/Application.h>`, used solely at `FwRibbon.cpp:395-406` for `GetParameterGroupByPath`/`GetInt`/`SetInt` preference read/write — explicitly permitted (standard Gui ParameterGrp practice), not App-state mutation. Commands fire via `Gui::Command::addTo()` / `Gui::CommandManager` only. |
| T-02-05 | DoS/usability | CLOSED | `FwRibbon::installOverflowButton()` (`FwRibbon.cpp:318`, `setCornerWidget` at :344) pins a "More commands…" overflow; `rebuildOverflowMenu()` (`:347`) builds from `manager.getAllCommands()` (`:365`). Shortcuts survive `menuBar()->hide()` because overflow entries reuse each command's existing `QAction` via `addTo()` (`:381`). Test `test_OverflowCornerWidgetPresent` (`FwRibbonWidget.cpp:287`). |
| T-02-06 | Tampering | CLOSED | `FwLayout::hideStockChrome()` (`FwLayout.cpp:258`) snapshots hidden toolbar names + menu-bar visibility + native-menu flag into FreeWorks statics; `restoreStockChrome()` (`:303`) replays via `ToolBarManager::State::RestoreDefault` (`:317`) and `setVisible(snapshot)` (`:326`). `FwWorkbench::activated()` hides (`FwWorkbench.cpp:63`), `deactivated()` restores (`:70`). Test `test_ChromeHideRestoreRoundTrip` (`FwRibbonWidget.cpp:267`). |
| T-02-07 | Tampering | CLOSED | Ribbon wrapped in real `Gui::ToolBar` objectName `Fw_RibbonToolBar` (`FwLayout.cpp:207-208`), added via `mw->addToolBar(Qt::TopToolBarArea, wrapper)` (`:214`) → QMainWindow `saveState/restoreState` serializes it. Selected tab persisted separately via `ParameterGrp::SetInt`/`GetInt` (`FwRibbon.cpp:386-410`). Tests `test_ToolBarStateRoundTripRestoresArea` (`FwRibbonWidget.cpp:309`), `test_TabStateRoundTripRestoresIndex` (`:334`). |
| T-02-08 | Tampering | CLOSED | Sketch identity by `vp.getTypeId().getName() == "SketcherGui::ViewProviderSketch"` (`FwRibbonContext.cpp:54,102,167`). `grep -rn "Mod/Sketcher" src/Gui/FreeWorks/` → only the documentary comment at `FwRibbonContext.h:56` stating no such include exists; zero actual `#include <Mod/Sketcher/...>`. Test `isSketchTypeMatchesOnlyTheContractLiteral` (`FwRibbon.cpp:168`). |
| T-02-09 | DoS/dangling-ref | CLOSED | `FwRibbonContext` stores `fastsignals::scoped_connection inEditConn_/resetEditConn_` (`FwRibbonContext.h:171-172`), released in `disconnect()` + dtor (`FwRibbonContext.cpp:65-71,124-129`). `connect()` idempotent — reassigns scoped connections (`:87-89`). Exactly one context via `FwLayout::s_ribbonContext` unique_ptr + `bindRibbonContext` (`FwLayout.cpp:58,222-234`). `QPointer<FwRibbon> ribbon_` guards post-teardown signals (`FwRibbonContext.h:160`). `unmountRibbon()` resets context BEFORE tearing down ribbon (`FwLayout.cpp:236-256`). Tests at `FwRibbon.cpp:194,236,247`, `FwRibbonWidget.cpp:249,362`. |
| T-02-10 | Tampering | CLOSED | `grep -c "activeDialog" FwRibbonContext.cpp` = 0; `grep -rn "QTimer" src/Gui/FreeWorks/` = 0. Switch is event-driven via `app->signalInEdit.connect(...)` / `signalResetEdit.connect(...)` (`FwRibbonContext.cpp:99,111`). No polling loop. |

### ACCEPT — accepted-risk log

| Threat | Category | Rationale (verified) | Verdict |
|--------|----------|----------------------|---------|
| T-02-04 | Spoofing/Input Validation | Command-ID strings are developer-authored compile-time constants (`FwRibbonMap.cpp:51 constexpr auto kRows = std::to_array<FwRibbonRow>(...)`) or registry-sourced; no user input. Every `getCommandByName` call path is null-checked: `FwRibbon.cpp:121`, `:236`, `:290`, and the `getAllCommands()` loop null-checks each entry `:366`. Gaps recorded as `// gap:` comments, not fabricated IDs (`FwRibbonMap.cpp:144-147`). Tests `bogusIdResolvesToNull` (`FwRibbon.cpp:91`), `SkipsUnresolvedIds` (`FwRibbonWidget.cpp:86`). | ACCEPTED — valid |
| T-02-DC | Tampering | Ribbon / overflow / auto-switch buttons are thin triggers placing only real registered command IDs via `cmd->addTo()`; no synthesized command is constructed. Destructive commands keep their existing downstream confirmation (no new path — `FwRibbon.cpp:363,381`). Tab auto-switch fires NO command — it only calls `setCurrentTab`/`setCurrentIndex` (`FwRibbonContext.cpp:106-107,118-120`). | ACCEPTED — valid |

### MOOT — non-existence confirmed

| Threat | Category | Verdict | Evidence |
|--------|----------|---------|----------|
| T-02-SC | Supply chain | NOT APPLICABLE | SPIKE.md verdict "native committed" (`SPIKE.md:126`); SARibbon fallback "NOT triggered" (`:148`). `src/3rdParty/SARibbon` does not exist. No `add_subdirectory(SARibbon)` / no SARibbon submodule / no Pixi dep. Only repo "SARibbon" mentions are doc comments recording the shelved fallback. Vendor-vet gate applies only if the native spike failed; it did not. |

---

## Unregistered flags

None. No `## Threat Flags` section appears in any of `02-01-SUMMARY.md`..`02-04-SUMMARY.md`. The single "flag" string in the summaries (`02-03-SUMMARY.md:114`) refers to the `m_suppressTabStateSave` boolean guard, not a new attack surface. No new attack surface introduced during implementation lacks a threat mapping.

---

## Informational (out of Phase-02 scope — not a finding)

`tests/src/Gui/FwWorkbench.cpp` (a Phase-01 artifact, last touched commit `22c0e3b21e`, NOT in the Phase-02 diff) contains two non-allow-listed "SolidWorks" tokens at `:31` (identifier `kSolidWorksNavStyle`, whose value is the allow-listed type-name string) and `:111`/`:118` (test name `navigationStyleDefaultsToSolidWorksWhenUnset`). These are pre-existing and belong to Phase 01's audit surface, not Phase 02. The Phase-02 string-leak gate over `src/Gui/FreeWorks/` and the new Phase-02 test files is clean. Flagged here only for traceability; recommend Phase 01 re-audit confirm the gate's scope covers `tests/src/Gui/` or that these test-only identifiers were intentionally accepted.

---

## Accepted-risk register (standing)

- **T-02-04** — Command-ID resolution relies on developer-authored/registry constants with universal `getCommandByName` null-checking; no user-supplied IDs. Accepted (ASVS L1, no untrusted input).
- **T-02-DC** — Ribbon/overflow buttons are thin triggers of pre-registered commands; destructive confirmations remain downstream; tab auto-switch fires no command. Accepted (no new unconfirmed destructive path).
