---
status: partial
phase: 03-featuremanager-design-tree
source: [03-01-SUMMARY.md, 03-02-SUMMARY.md, 03-03-SUMMARY.md]
started: 2026-06-14T18:00:00Z
updated: 2026-06-14T18:10:00Z
---

## Current Test

[testing complete]

## Tests

### 1. FeatureManager design tree (scoped to active Body) — TREE-01
expected: Left Fw_FeatureManager dock shows the active Body's design tree only (non-active Bodies hidden), 16px indent, "No active Body" empty state when none active.
result: blocked
blocked_by: release-build
reason: "No build tree / live GUI in this environment — cannot click-test. Logic source-verified (REAL two-Body QTEST scoping render, 03-01); live demo routed to FwFeatureTree_SPIKE_LIVE_CHECKLIST.md."

### 2. Origin planes display as Front/Top/Right — TREE-01/TREE-03
expected: The three origin planes (XY/XZ/YZ) display as "Front Plane"/"Top Plane"/"Right Plane" (display-only; the object Label is never rewritten).
result: blocked
blocked_by: release-build
reason: "No build tree / live GUI — cannot click-test. Delegate remap source-verified (03-02 QTEST, Labels-unchanged asserted); A1 plane-correspondence FEEL routed to SPIKE_LIVE_CHECKLIST.md item (6)."

### 3. F2 inline rename — TREE-01
expected: Pressing F2 on a feature row opens inline rename (the inherited Gui::TreeWidget relabel action); Enter commits the new Label.
result: blocked
blocked_by: release-build
reason: "No build tree / live GUI — cannot click-test. Inherited F2/Return action presence source-verified (03-02 QTEST test_ConstructsAsTreeWidgetWithF2Action)."

### 4. Drag-reorder validity affordance — TREE-04
expected: Dragging a feature to a valid position shows the 2px insertion line and reorders in one transaction; dragging to an invalid position (e.g. a child before its parent) shows the forbidden cursor, no insertion line, and refuses the drop (no transaction).
result: blocked
blocked_by: release-build
reason: "No build tree / live GUI — cannot click-test. Drop-gate BLOCK logic (no transaction on invalid) source-verified (03-01/03-02 GTest); insertion-line/forbidden-cursor FEEL routed to SPIKE_LIVE_CHECKLIST.md item (4)."

### 5. Rollback bar — drag up to roll back — TREE-02
expected: Grab the 4px band at the tip boundary (8px SizeVerCursor grab zone) and drag it up; the bar snaps to the nearest preceding solid feature, features below grey out in the tree, and the 3D view shows the rolled-back result (existing ViewProviderBody tip display). One Ctrl+Z restores.
result: blocked
blocked_by: release-build
reason: "No build tree / live GUI — cannot click-test. Link-free snap + set-Tip→mustExecute()==1 + one-undo-restores source-verified (03-03 GTest); band paint + SizeVerCursor source-verified (03-03 QTEST); 3D suppress-below FEEL routed to SPIKE_LIVE_CHECKLIST.md item (1)."

### 6. Rollback reversibility — drag down / Roll to End — TREE-02
expected: Dragging the bar back down moves the tip forward feature-by-feature; "Roll to End" restores the tip to the last solid feature (model fully restored).
result: blocked
blocked_by: release-build
reason: "No build tree / live GUI — cannot click-test. Roll-to-End forward restore source-verified (03-03 GTest); live reversibility FEEL routed to SPIKE_LIVE_CHECKLIST.md item (2)."

### 7. Roll Back / Roll Forward / Roll to End context actions — TREE-02
expected: Right-clicking a feature offers "Roll Back" (tip → clicked feature), "Roll Forward" (tip → next solid), and "Roll to End"; each fires the real Body.Tip move. The actions are greyed when there is no active Body.
result: blocked
blocked_by: release-build
reason: "No build tree / live GUI — cannot click-test. Exact action labels + fire path source-verified (03-03 QTEST + grep); no-Body greying added in 734979cf78."

### 8. Insert-at-bar mid-history insertion — TREE-02
expected: Inserting a feature at the rollback bar places it after the current tip; a solid insert becomes the new tip (SW-like mid-history insertion), a non-solid (sketch/datum) leaves the tip unchanged.
result: blocked
blocked_by: release-build
reason: "No build tree / live GUI — cannot click-test. Python-only insertObject path + explicit Tip policy (solid→tip, non-solid→unchanged) source-verified (03-03 GTest)."

## Summary

total: 8
passed: 0
issues: 0
pending: 0
skipped: 0
blocked: 8

## Gaps

[none — blocked tests are prerequisite gates (need a build tree / live GUI), not code
issues. Every success criterion above is source-verified by the cited GTest/QTest +
grep acceptance criteria; the live FEEL residue is recorded as open obligations in
src/Gui/FreeWorks/SPIKE_LIVE_CHECKLIST.md (Phase-3 section) and gates the milestone
parity sign-off, not this phase's code completion.]
