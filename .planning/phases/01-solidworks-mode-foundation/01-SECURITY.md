---
phase: 01
slug: solidworks-mode-foundation
status: verified
threats_open: 0
threats_total: 13
threats_closed: 13
asvs_level: 1
block_on: high
created: 2026-06-07
---

<!-- SPDX-License-Identifier: LGPL-2.1-or-later -->

# SECURITY.md — Phase 01: solidworks-mode-foundation

**Audit mode:** Verify declared mitigations exist and are effective in implemented code (register is plan-time authored; no blind vulnerability scan).
**ASVS Level:** 1
**block_on:** high
**Audited:** 2026-06-07
**Result:** SECURED (13/13 threats CLOSED) with 1 informational unregistered flag and 1 advisory note.

---

## Threat Verification Summary

| Threat ID | Category | Disposition | Status | Evidence |
|-----------|----------|-------------|--------|----------|
| T-01-01 | Tampering | mitigate | CLOSED | `src/Gui/CMakeLists.txt:17` — exactly one `add_subdirectory(FreeWorks)  # SW-FORK HOOK`; `grep -c == 1` confirmed. |
| T-01-02 | Information Disclosure | mitigate | CLOSED (caveat) | `FwLayout.cpp:84` uses `Gui::DockWindowManager::instance()` only (Gui-layer); zero App includes in FwLayout. Module-wide "zero App includes" superseded by one sanctioned read in `FwNavigationDefault.cpp:27` (`<App/Application.h>` for the preference store, per Plan 02). True boundary (no GUI state into App documents) enforced by the headless gate — see T-01-07. |
| T-01-03 | Spoofing/Legal | accept (scaffold) / mitigate (Plan 04) | CLOSED | Icon `Resources/icons/FreeWorksWorkbench.svg` is recreated-original (neutral "FW" mark, no proprietary content). `ASSET_PROVENANCE.md` row present; `fw-provenance-guard.sh` enforced in CI (T-01-10). |
| T-01-04 | Tampering | mitigate | CLOSED | `FwNavigationDefault.cpp:45-55` writes `NavigationStyle` only when `GetASCII(...,"")` is empty; line 54 marked `// SW-FORK HOOK`. No-clobber proven by GTest `navigationStyleDoesNotClobberUserChoice` (`tests/src/Gui/FwWorkbench.cpp:122-131`). |
| T-01-05 | Repudiation/Legal | mitigate | CLOSED | Sole "SolidWorks" token is `Gui::SolidWorksNavigationStyle` (`FwNavigationDefault.cpp:54`). `fw-string-leak-grep.sh` passes (exit 0) on clean tree, fails on injected `SolidWorksWorkbench` (verified live); allow-list is exactly this token. |
| T-01-06 | Denial of Service | accept | CLOSED | `MACOS_NAV_PROFILE.md` documents modifier-emulated-MMB default (Option+left-drag), `Gui::GestureNavigationStyle` one-click alternative, full mapping table, and Mac-hardware spike checklist (§4). Checkpoint approved; live-hardware feel deferred to spike checklist (accepted risk). |
| T-01-07 | Information Disclosure | mitigate | CLOSED | `sub_fwHeadlessCompat.yml:185-202` extracts App-layer `Document.xml` from the saved `.FCStd` and fails on `Fw_\|FreeWorksGui\|NavigationStyle\|SolidWorksNavigationStyle\|FwMacNavProfile`. Wired into `CI_primary.yml:74-79` and blocks WrapUp. |
| T-01-08 | Denial of Service | mitigate | CLOSED | FreeWorks compiles into FreeCADGui via `target_sources(FreeCADGui PRIVATE ...)` (`CMakeLists.txt:42`). Per-OS matrix: `sub_buildUbuntu.yml` (CI_primary), `sub_buildPixi.yml`+`sub_buildWindows.yml` (CI_approved, label-gated per upstream convention). Manual leg: `TRIOS_LAUNCH_CHECKLIST.md`. |
| T-01-09 | Tampering | mitigate | CLOSED | `sub_fwHeadlessCompat.yml:159-178` opens the FreeWorks-saved `.FCStd` through the unmodified upstream headless path, recomputes, asserts `HEADLESS_OK` + exit 0. |
| T-01-10 | Tampering/Supply-chain | mitigate | CLOSED | `fw-provenance-guard.sh` exits 0 on listed tree, exits 1 on unlisted image (verified live). CI runs guard + self-test: `sub_fwForkGuards.yml:80-94`, wired via `CI_primary.yml:81-86`. |
| T-01-11 | Spoofing/Legal | mitigate | CLOSED | `fw-string-leak-grep.sh` exit 0 on clean tree; exit 1 on injected token (verified live). Runs in `sub_fwForkGuards.yml:96-99`. |
| T-01-12 | Tampering/Supply-chain | mitigate | CLOSED (scope caveat) | `fw-sync-upstream.sh` runs against pinned base `768e23709...` (self-checks ancestry + pristine tree), enumerates markers, asserts no unmarked shared-file diff. PASS verified live. **Caveat:** the diff is scoped to `-- src/`, so edits to shared upstream files OUTSIDE `src/` are not audited (see Advisory A1). Runs in CI: `sub_fwForkGuards.yml:101-104`. |
| T-01-SC | Tampering | n/a | CLOSED | No npm/pip/cargo installs in this phase (native Qt6 Widgets only). No package-manager legitimacy gate required; confirmed no new dependency manifests added by the phase. |

---

## Live Verification Evidence

| Check | Command | Result |
|-------|---------|--------|
| Single marked CMake hook | `grep -c 'add_subdirectory(FreeWorks)' src/Gui/CMakeLists.txt` | `1` |
| Leak grep (clean tree) | `bash tools/fw-string-leak-grep.sh` | exit 0 |
| Leak grep (injected token) | `bash tools/fw-string-leak-grep.sh /tmp/fwleak-test` | exit 1 (rejects) |
| Only allowed SW token | `grep -rnI SolidWorks src/Gui/FreeWorks \| grep -v Gui::SolidWorksNavigationStyle` | empty |
| Provenance guard (clean) | `bash tools/fw-provenance-guard.sh` | exit 0 |
| Provenance guard (unlisted) | `bash tools/fw-provenance-guard.sh /tmp/fwprov-test` | exit 1 (rejects) |
| Sync drill (offline/CI portion) | `FW_SYNC_OFFLINE=1 bash tools/fw-sync-upstream.sh` | PASS (exit 0) |
| SW-FORK HOOK markers under src/ | `grep -rn 'SW-FORK HOOK' src/` | 3 lines / 2 files (1 shared: `src/Gui/CMakeLists.txt`) |
| App includes in module | `grep -rn 'include <App/' src/Gui/FreeWorks/` | 1 (`FwNavigationDefault.cpp` — sanctioned preference read) |
| Both CI gates wired | `grep -E 'sub_fwForkGuards\|sub_fwHeadlessCompat' .github/workflows/CI_primary.yml` | both present, both in WrapUp `needs` |

---

## Unregistered Flags (informational — not blockers)

### UF-1: `FwMacNavProfile` preference key (new attack surface, covered)

`FwNavigationDefault.cpp:70-76` writes a new FreeWorks-namespaced preference key
`FwMacNavProfile` (value `"ModifierEmulatedMMB"`) on macOS, when unset, marked
`// SW-FORK HOOK`. This key did not exist in the plan-time threat register or in
any SUMMARY `## Threat Model Compliance` section, so it is new attack surface
relative to the threat model.

- **Risk posture:** LOW. The key is written to the user preference store
  (`User parameter:BaseApp/Preferences/View`), not to any `.FCStd` App-layer
  document. It is currently write-only intent (nothing consumes it this phase —
  confirmed by 01-VERIFICATION.md WR-03 fix).
- **Already covered:** The headless App/Gui-leak gate explicitly greps for
  `FwMacNavProfile` (`sub_fwHeadlessCompat.yml:197`), so any leak of this key
  into App-layer persisted state would block CI under T-01-07.
- **Action:** None required for this phase. Map `FwMacNavProfile` into the threat
  register when Phase 6 (gestures) makes it load-bearing.

---

## Advisory Notes

### A1: `fw-sync-upstream.sh` diff scope is `src/` only (T-01-12 partial coverage)

The merge-discipline drill enforces the `// SW-FORK HOOK` invariant only over
files under `src/` (`fw-sync-upstream.sh:212` `git diff ... -- src/`). The phase
introduced three real edits to **pre-existing upstream files outside `src/`** that
are therefore not audited by the drill and carry no marker:

- `tests/src/Gui/CMakeLists.txt` — adds `FwWorkbench.cpp` to `Gui_tests_run` (planned, Plan 03).
- `.github/workflows/CI_primary.yml` — adds the two FreeWorks CI job references (planned, Plans 03/04).
- `.gitignore` — adds `!/.planning/` (incidental tooling edit, not in any plan).

These are all in-scope/benign edits for this phase, and the drill's stated
expectation ("the two expected markers") is met for `src/`. T-01-12 is marked
CLOSED because the declared mitigation (block on unmarked shared-file diff under
the C++ source tree, against a pinned pristine base with ancestry self-checks)
functions correctly. **However**, the mitigation text ("all shared-file touches")
is broader than the drill's `src/`-only enforcement. Recommendation (non-blocking
for ASVS-L1 / `block_on: high`): widen the drill's diff scope to the whole tree
(excluding `.planning/`) so CI/test/build-config edits to upstream files are also
required to be intentional/marked, or explicitly document that the
`// SW-FORK HOOK` convention applies to C++ shared code only.

### A2: T-01-02 literal phrasing vs. one sanctioned App include

The T-01-02 mitigation says "Module has zero App-layer includes." The
implemented module has exactly one App include (`FwNavigationDefault.cpp:27`,
`<App/Application.h>`), which Plan 02 explicitly directs for reading the
`NavigationStyle` preference via `App::GetApplication().GetParameterGroupByPath`.
This is a **read of shared configuration**, not a leak of GUI state into App
documents. The security-relevant invariant — no GUI/FreeWorks state in App-layer
persisted documents — is intact and is enforced by the headless gate (T-01-07).
The named component in the mitigation (`FwLayout`) does observe via Gui-layer
getters only and has zero App includes. CLOSED on the security invariant; the
"zero includes" phrasing is superseded by the sanctioned Plan 02 preference read.

---

## Accepted Risks Log

| ID | Threat | Accepted Risk | Justification | Compensating Control |
|----|--------|---------------|---------------|----------------------|
| T-01-06 | macOS missing physical MMB UX | Live ergonomic feel of the modifier-emulated-MMB chords is not validated in CI | No Mac build tree / hardware in the execution environment; DoS/UX-only, not a data/security boundary | `MACOS_NAV_PROFILE.md` documents the default profile, the `Gui::GestureNavigationStyle` one-click alternative, and a real-hardware spike checklist (§4) carried to Phase 6; blocking checkpoint approved by the user. |

---

## Conclusion

All 13 threats in the plan-time register resolve to **CLOSED**. The four CI/CD
guards (headless App/Gui-leak gate, asset-provenance guard, "SolidWorks"-leak
grep, pinned-commit sync drill) are present, function as claimed (verified by live
positive + negative runs), and are wired into `CI_primary.yml`. Two non-blocking
advisories (A1: sync-drill `src/`-only scope; A2: one sanctioned App include) and
one informational unregistered flag (UF-1: `FwMacNavProfile`) are recorded for
follow-up but do not block this phase under ASVS-L1 / `block_on: high`.
