---
phase: 01-solidworks-mode-foundation
plan: 04
type: execute
wave: 3
depends_on: [01-01, 01-03]
files_modified:
  - ASSET_PROVENANCE.md
  - tools/fw-provenance-guard.sh
  - tools/fw-string-leak-grep.sh
  - tools/fw-sync-upstream.sh
  - .github/workflows/sub_fwForkGuards.yml
  - .github/workflows/CI_primary.yml
autonomous: false
requirements: [SHELL-02]
must_haves:
  truths:
    - "A CI asset-provenance guard rejects any binary image lacking a source entry in ASSET_PROVENANCE.md"
    - "A 'SolidWorks'-string leak grep over src/Gui/FreeWorks/ fails on any 'SolidWorks' identifier except the allow-listed upstream Gui::SolidWorksNavigationStyle"
    - "A scripted upstream-sync drill runs against a pinned upstream commit and completes, asserting all unavoidable shared-file touches are greppable via // SW-FORK HOOK"
    - "ASSET_PROVENANCE.md records one row per binary image with its original source"
  artifacts:
    - path: "ASSET_PROVENANCE.md"
      provides: "Provenance ledger: one row per binary image -> original source (recreated-original)"
      contains: "Provenance"
    - path: "tools/fw-provenance-guard.sh"
      provides: "Exits non-zero when any binary image lacks a provenance row"
      contains: "ASSET_PROVENANCE.md"
    - path: "tools/fw-string-leak-grep.sh"
      provides: "Fails on 'SolidWorks' in src/Gui/FreeWorks/ except allow-listed Gui::SolidWorksNavigationStyle"
      contains: "Gui::SolidWorksNavigationStyle"
    - path: "tools/fw-sync-upstream.sh"
      provides: "Pinned-commit upstream-sync drill asserting // SW-FORK HOOK greppability"
      contains: "SW-FORK HOOK"
    - path: ".github/workflows/sub_fwForkGuards.yml"
      provides: "CI gate running provenance guard + leak grep (workflow_call)"
      contains: "workflow_call"
  key_links:
    - from: ".github/workflows/CI_primary.yml"
      to: ".github/workflows/sub_fwForkGuards.yml"
      via: "uses: ./.github/workflows/sub_fwForkGuards.yml"
      pattern: "sub_fwForkGuards"
    - from: "tools/fw-provenance-guard.sh"
      to: "ASSET_PROVENANCE.md"
      via: "diff added binary images vs ledger rows"
      pattern: "ASSET_PROVENANCE.md"
---

<objective>
Establish the merge-safety, asset-provenance, and trademark-avoidance foundation that keeps the fork mergeable, asset-clean, and legally safe. Create the `ASSET_PROVENANCE.md` ledger and a CI guard that rejects any unlisted binary image; a "SolidWorks"-string leak grep over `src/Gui/FreeWorks/` (allow-listing only the upstream `Gui::SolidWorksNavigationStyle` type-name); and the scripted `tools/fw-sync-upstream.sh` upstream-sync drill against a pinned commit that asserts every unavoidable shared-file touch is greppable via `// SW-FORK HOOK`.

Purpose: SHELL-02 cross-cutting foundation — keeps later phases mergeable against a moving upstream `main`, asset-clean, and trademark-safe (D-03).
Output: `ASSET_PROVENANCE.md`, `tools/fw-provenance-guard.sh`, `tools/fw-string-leak-grep.sh`, `tools/fw-sync-upstream.sh`, and the `sub_fwForkGuards.yml` CI gate slotted into `CI_primary.yml`.
</objective>

<execution_context>
@$HOME/.claude/gsd-core/workflows/execute-plan.md
@$HOME/.claude/gsd-core/templates/summary.md
</execution_context>

<context>
@.planning/PROJECT.md
@.planning/ROADMAP.md
@.planning/STATE.md
@.planning/phases/01-solidworks-mode-foundation/01-SKELETON.md
@.planning/phases/01-solidworks-mode-foundation/01-PATTERNS.md
@.planning/phases/01-solidworks-mode-foundation/01-CONTEXT.md
@.planning/phases/01-solidworks-mode-foundation/01-01-SUMMARY.md
</context>

<artifacts_produced>
## Artifacts this phase produces (Plan 04)

| Symbol / Path | Kind |
|---|---|
| `ASSET_PROVENANCE.md` | provenance ledger (one row per binary image) |
| `tools/fw-provenance-guard.sh` | CI script (exits non-zero on unlisted asset) |
| `tools/fw-string-leak-grep.sh` | CI script ("SolidWorks" leak grep, allow-lists `Gui::SolidWorksNavigationStyle`) |
| `tools/fw-sync-upstream.sh` | upstream-sync drill (pinned commit + `// SW-FORK HOOK` enumeration) |
| `.github/workflows/sub_fwForkGuards.yml` | CI gate (workflow_call) |
</artifacts_produced>

<tasks>

<task type="auto">
  <name>Task 1: ASSET_PROVENANCE.md ledger + provenance-guard script</name>
  <read_first>
    - 01-PATTERNS.md "No Analog Found" -> `ASSET_PROVENANCE.md` (greenfield; author one row per binary image -> original source, per RESEARCH Pitfall 5).
    - 01-PATTERNS.md section ".github/workflows/ provenance + headless gates" — asset-provenance guard diffs added binary images vs `ASSET_PROVENANCE.md`, fails if any unlisted (SC4a).
    - 01-SKELETON.md "Asset/legal" decision row (workbench icon = recreated original art; row required before it lands; no verbatim SW assets).
    - 01-CONTEXT.md D-03 / Legal constraint.
  </read_first>
  <action>
    Create `ASSET_PROVENANCE.md` (SPDX header line 1) with a table: one row per binary image asset (path, asset type, "recreated-original", source/author, license, date). Include the row for the FreeWorks workbench icon referenced by Plan 01's `Resources/FreeWorks.qrc` (recreated original art — no proprietary SW asset). Create `tools/fw-provenance-guard.sh` (SPDX line 1) that enumerates binary image files under the repo's tracked image paths (at minimum `src/Gui/FreeWorks/Resources/`), and for each image checks that its path appears as a row in `ASSET_PROVENANCE.md`; exit non-zero (with a clear message naming the offending file) if any binary image lacks a row. The script must be deterministic and runnable locally and in CI. Include a self-test path the script supports (e.g. accepts a directory arg) so the CI gate can prove it rejects an unlisted asset.
  </action>
  <verify>
    <automated>test -f ASSET_PROVENANCE.md &amp;&amp; test -x tools/fw-provenance-guard.sh &amp;&amp; bash tools/fw-provenance-guard.sh &amp;&amp; mkdir -p /tmp/fwprov-test &amp;&amp; printf 'PNG' > /tmp/fwprov-test/unlisted.png &amp;&amp; ! bash tools/fw-provenance-guard.sh /tmp/fwprov-test</automated>
  </verify>
  <acceptance_criteria>
    - `ASSET_PROVENANCE.md` has one row per binary image, including the FreeWorks workbench icon (recreated-original).
    - `tools/fw-provenance-guard.sh` exits 0 when all tracked images are listed.
    - `tools/fw-provenance-guard.sh` exits non-zero when pointed at a directory containing an unlisted image.
  </acceptance_criteria>
  <done>The provenance ledger exists with a row per binary image, and the guard rejects any unlisted asset (proven by the self-test exiting non-zero).</done>
</task>

<task type="auto">
  <name>Task 2: "SolidWorks"-string leak grep (allow-listing only Gui::SolidWorksNavigationStyle)</name>
  <read_first>
    - 01-PATTERNS.md section ".github/workflows/" — "SolidWorks"-leak grep fails on `SolidWorks` in new `src/Gui/FreeWorks/` identifiers/strings, allow-listing the single `"Gui::SolidWorksNavigationStyle"` reference (Pitfall 4).
    - 01-CONTEXT.md D-03 — internal descriptive comments referencing SW *behavior* are fine; identifiers/paths/user-facing strings are not; the ONLY allowed "SolidWorks" identifier is the upstream type-name string.
    - Plan 02 `FwNavigationDefault.cpp` is the sole legitimate `Gui::SolidWorksNavigationStyle` occurrence.
  </read_first>
  <action>
    Create `tools/fw-string-leak-grep.sh` (SPDX line 1) that greps `src/Gui/FreeWorks/` for the token `SolidWorks` in code identifiers, paths, and user-facing strings, and exits non-zero if any occurrence is found that is NOT the allow-listed upstream type name `Gui::SolidWorksNavigationStyle`. The allow-list must be exactly that one token (so a stray `SolidWorksWorkbench` or a user-facing "SolidWorks" label still fails). The script names each offending file:line. Per D-03, allow internal `//`-comments that describe SolidWorks *behavior* if your grep strategy can distinguish them — otherwise scope the grep to identifiers/strings and document the convention in the script header. Runnable locally and in CI.
  </action>
  <verify>
    <automated>test -x tools/fw-string-leak-grep.sh &amp;&amp; grep -q "Gui::SolidWorksNavigationStyle" tools/fw-string-leak-grep.sh &amp;&amp; bash tools/fw-string-leak-grep.sh</automated>
  </verify>
  <acceptance_criteria>
    - `tools/fw-string-leak-grep.sh` passes (exit 0) on the current FreeWorks tree where the only "SolidWorks" token is `Gui::SolidWorksNavigationStyle`.
    - It exits non-zero if any other "SolidWorks" identifier / user-facing string is introduced under `src/Gui/FreeWorks/`.
  </acceptance_criteria>
  <done>The leak grep enforces D-03: zero "SolidWorks" identifiers in new FreeWorks code except the single upstream type-name reference.</done>
</task>

<task type="auto">
  <name>Task 3: Upstream-sync drill script + fork-guards CI gate</name>
  <read_first>
    - 01-PATTERNS.md "No Analog Found" -> `tools/fw-sync-upstream.sh` (greenfield; author from RESEARCH SC5: pinned-commit sync, mirror branch, `grep -rn "SW-FORK HOOK" src/`).
    - 01-PATTERNS.md section ".github/workflows/" — unmarked-shared-edit grep asserts no `MainWindow.cpp`/shared diffs outside `// SW-FORK HOOK` (SC1/SC5); `sub_lint.yml:30-40` `workflow_call` shape for the gate.
    - 01-SKELETON.md "Merge discipline" decision row — exactly two expected `// SW-FORK HOOK` touches: the CMake `add_subdirectory` (Plan 01) and the nav-default write (Plan 02).
    - 01-CONTEXT.md Established Patterns (additive module + marked shared edits).
  </read_first>
  <action>
    Create `tools/fw-sync-upstream.sh` (SPDX line 1): a scripted upstream-sync drill that (1) takes/uses a PINNED upstream commit SHA, (2) creates a mirror branch and merges/rebases the fork against the pin, (3) enumerates every shared-file touch via `grep -rn "SW-FORK HOOK" src/` and asserts each unavoidable shared-file edit is marked (the two expected: `src/Gui/CMakeLists.txt` add_subdirectory and the `FwNavigationDefault.cpp` nav write), (4) fails if any shared-file diff outside `src/Gui/FreeWorks/` lacks a `// SW-FORK HOOK` marker, and (5) reports completion. The script must run against the pin and complete (hours-not-days drill). Create `.github/workflows/sub_fwForkGuards.yml` (SPDX line 1) as a `workflow_call` job (mirroring `sub_lint.yml`) that runs `fw-provenance-guard.sh` and `fw-string-leak-grep.sh` (and optionally the marker-enumeration portion of the sync drill). Slot it into `.github/workflows/CI_primary.yml` via `uses: ./.github/workflows/sub_fwForkGuards.yml`.
  </action>
  <verify>
    <automated>test -x tools/fw-sync-upstream.sh &amp;&amp; grep -q "SW-FORK HOOK" tools/fw-sync-upstream.sh &amp;&amp; test -f .github/workflows/sub_fwForkGuards.yml &amp;&amp; grep -q "workflow_call" .github/workflows/sub_fwForkGuards.yml &amp;&amp; grep -q "sub_fwForkGuards" .github/workflows/CI_primary.yml</automated>
  </verify>
  <acceptance_criteria>
    - `tools/fw-sync-upstream.sh` runs against a pinned upstream commit, enumerates `// SW-FORK HOOK` markers, and fails on any unmarked shared-file diff outside `src/Gui/FreeWorks/`.
    - `sub_fwForkGuards.yml` is a `workflow_call` job running the provenance guard + leak grep, referenced from `CI_primary.yml`.
  </acceptance_criteria>
  <done>The upstream-sync drill completes against a pin with all unavoidable shared-file touches greppable via // SW-FORK HOOK, and the fork-guards CI gate runs provenance + leak checks in CI.</done>
</task>

<task type="checkpoint:human-verify" gate="blocking-human">
  <name>Task 4: Confirm trademark/asset discipline + sync-drill outcome</name>
  <what-built>The provenance guard, "SolidWorks"-leak grep, and pinned-commit upstream-sync drill are scripted and wired into CI (sub_fwForkGuards). The legal/trademark posture (D-03) and the merge-safety claim require a human confirm before this foundation is trusted by later phases.</what-built>
  <how-to-verify>
    1. Run `bash tools/fw-provenance-guard.sh` (expect exit 0) and the unlisted-asset self-test (expect non-zero).
    2. Run `bash tools/fw-string-leak-grep.sh` (expect exit 0; confirm `Gui::SolidWorksNavigationStyle` is the only "SolidWorks" token).
    3. Run `bash tools/fw-sync-upstream.sh` against the pinned upstream commit; confirm it completes and enumerates exactly the two expected `// SW-FORK HOOK` markers (`src/Gui/CMakeLists.txt`, `FwNavigationDefault.cpp`).
    4. Confirm `ASSET_PROVENANCE.md` lists the workbench icon as recreated-original (no proprietary SW asset).
  </how-to-verify>
  <resume-signal>Type "approved" if all three guards pass, the sync drill completes against the pin, and the asset ledger is clean; otherwise describe the gap.</resume-signal>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| untrusted upstream pull -> fork | Pulling a moving upstream main can silently introduce unmarked shared-file divergence |
| recreated asset -> repo / distribution | An unlisted or proprietary-derived image is a legal/supply-chain risk |
| new FreeWorks code -> user-facing surface | A leaked "SolidWorks" identifier/string is a trademark risk (D-03) |

## STRIDE Threat Register

| Threat ID | Category | Component | Disposition | Mitigation Plan |
|-----------|----------|-----------|-------------|-----------------|
| T-01-10 | Tampering / Supply-chain | binary image assets | mitigate | `fw-provenance-guard.sh` rejects any image lacking an `ASSET_PROVENANCE.md` row; self-test proves rejection; runs in CI |
| T-01-11 | Spoofing / Legal | "SolidWorks" identifiers (D-03) | mitigate | `fw-string-leak-grep.sh` fails on any "SolidWorks" token except allow-listed `Gui::SolidWorksNavigationStyle`; runs in CI |
| T-01-12 | Tampering | untrusted upstream pull / unmarked shared edits | mitigate | `fw-sync-upstream.sh` runs against a pinned commit and asserts all shared-file touches are `// SW-FORK HOOK`-greppable; blocks on unmarked diff |
| T-01-SC | Tampering | npm/pip/cargo installs | n/a | No package-manager installs in this phase; no legitimacy gate required |
</threat_model>

<verification>
- `bash tools/fw-provenance-guard.sh` exits 0; self-test on an unlisted image exits non-zero.
- `bash tools/fw-string-leak-grep.sh` exits 0 (only `Gui::SolidWorksNavigationStyle` present).
- `bash tools/fw-sync-upstream.sh` completes against the pinned commit, enumerating the two expected `// SW-FORK HOOK` markers.
- `sub_fwForkGuards.yml` referenced from `CI_primary.yml` and runs the guards.
</verification>

<success_criteria>
- CI asset-provenance guard rejects any binary image lacking a source entry in `ASSET_PROVENANCE.md`.
- "SolidWorks"-leak grep passes with only the allow-listed upstream type name.
- Scripted upstream-sync drill runs against a pinned commit and completes, all shared-file touches greppable via `// SW-FORK HOOK`.
</success_criteria>

<output>
Create `.planning/phases/01-solidworks-mode-foundation/01-04-SUMMARY.md` when done.
</output>
