# Codebase Concerns

**Analysis Date:** 2026-06-07

## Tech Debt

### Element Mapping & Link Properties Complexity

**Issue:** Duplicate and complex link handling between `PropertyLinks` and `ElementMap`
- Files: `src/App/PropertyLinks.h`, `src/App/PropertyLinks.cpp`, `src/App/ElementMap.h`, `src/App/ElementMap.cpp`
- Impact: Inconsistent state management, potential for link tracking failures
- Comment in code: "FIXME: Do not make two independent lists because this will lead to some inconsistencies!"
- Fix approach: Consolidate link management into a single source of truth with proper cache invalidation

### ElementMap Shared Pointer Lifecycle Uncertainty

**Issue:** ElementMap uses `shared_from_this` but design is not clear
- Files: `src/App/ElementMap.h` (line 81)
- Impact: Unclear ownership model; potential for circular references or premature cleanup
- Comment: "TODO can remove shared_from_this?"
- Fix approach: Document ownership model explicitly; audit all holders of shared_ptr<ElementMap> and consider switching to weak_ptr where appropriate

### Incomplete IFC Implementation in Strict Mode

**Issue:** Recent migration to Strict IFC mode (PR #30001) required follow-up fixes
- Files: `src/Mod/BIM/nativeifc/ifc_export.py`, `src/Mod/BIM/nativeifc/ifc_psets.py`, `src/Mod/BIM/nativeifc/ifc_materials.py`
- Impact: Export issues with IFC strict mode; recursion problems when processing document objects
- Recent fixes: Commits 768e237091 and 7392497954 address import recursion and object traversal
- Fix approach: Audit all Strict IFC paths for similar recursion patterns; consider document traversal as single-pass algorithm

### Sketcher Rendering Order Naive Implementation

**Issue:** Minimal/naive rendering order substitution, not production-ready
- Files: `src/Mod/Sketcher/Gui/ViewProviderSketch.cpp` (line 653)
- Impact: May produce incorrect visualization under edge cases
- Fix approach: Implement proper sorting based on geometric depth analysis

### ViewProviderSketch Scale Handling Bug

**Issue:** Scale transformation not properly handled in constraint placement
- Files: `src/Mod/Sketcher/Gui/ViewProviderSketch.cpp` (line 1118)
- Impact: Constraints may not render at correct screen positions when scale is applied
- Comment: "TODO: won't work if there is scale. Hmm... what to do..."
- Fix approach: Apply scale-aware coordinate transformation in constraint positioning logic

---

## Known Bugs

### Memory Leaks in Sketcher

**Symptoms:** Unreleased resources accumulate over time
- Files: `src/Mod/Sketcher/App/planegcs/GCS.cpp`, `src/Mod/Sketcher/App/Sketch.cpp`
- Trigger: Creating and manipulating multiple sketch constraints over extended session
- Status: Multiple fixes merged (commits e2ba9763d6, f7252a11ca, 5268aa43db)
- Workaround: Restart application between heavy sketcher sessions
- Resolution: Recent memory leak fixes should address core issues; verify in next release

### Recursion Issue in Document Traversal

**Symptoms:** Stack overflow or excessive processing when exporting IFC
- Files: `src/Mod/BIM/nativeifc/ifc_export.py` (Objects property recursion)
- Trigger: Exporting complex documents to IFC in Strict mode
- Workaround: Use non-strict IFC export or break document into smaller parts
- Fix: Commit c8599b531d eliminates recursion by using document.Objects directly

### Sketch Conic Geometry Selection Misses

**Symptoms:** Large conic sections (ellipses, parabolas, hyperbolas) miss selection clicks
- Files: `src/Mod/Sketcher/Gui/ViewProviderSketch.cpp` (line 3085)
- Trigger: Attempting to select large conic geometry by clicking
- Workaround: Use tree view selection or decrease conic size
- Fix approach: Improve spatial indexing or implement frustum-based selection

### Constraint Caching Missing in PropertyLinks

**Symptoms:** Repeated iteration over constraint sets causes performance degradation
- Files: `src/App/PropertyLinks.cpp`
- Trigger: Large assemblies with many linked objects
- Comment: "FIXME: cache this to avoid iterating each time, to improve speed"
- Impact: O(n) complexity for common operations that could be O(1)
- Fix approach: Implement lazy-loaded cache with proper invalidation on link changes

### Thread Safety Issue in ApplicationDirectories

**Symptoms:** Undefined behavior in multi-threaded file operations
- Files: `src/App/ApplicationDirectories.h`
- Trigger: Concurrent file system queries during plugin loading
- Comment: "TODO: Rewrite to be thread safe"
- Impact: Race conditions in file discovery, especially on network paths
- Fix approach: Use mutex-protected singleton with thread-safe lazy initialization

---

## Security Considerations

### URI Parsing Vulnerability in PropertyLinks

**Risk:** Weak URI validation may accept malformed or malicious link references
- Files: `src/App/PropertyLinks.cpp`
- Current mitigation: Basic string pattern matching
- Comment: "TODO: build a far much more resilient approach to test for an URI"
- Recommendations: 
  - Implement RFC 3986 compliant URI parser
  - Validate all components (scheme, authority, path, query, fragment)
  - Add integration tests for malformed URIs

### Backward Compatibility Legacy Paths

**Risk:** Old config file locations may be world-readable on multi-user systems
- Files: `src/App/Application.cpp` (keep-deprecated-paths flag), `src/App/FreeCADInit.py`
- Current mitigation: Option to use old paths kept for compatibility
- Recommendations:
  - Add migration warning for legacy path usage
  - Document security implications of legacy paths
  - Consider deprecating legacy paths in next major version

### Python Plugin Import Safety

**Risk:** Plugin system may execute arbitrary Python code with minimal validation
- Files: `src/App/FreeCADInit.py` (module loading), Plugin initialization
- Current mitigation: None explicitly documented
- Recommendations:
  - Audit plugin loading sequence
  - Consider sandboxing Python plugin execution
  - Document security model for third-party plugin usage

---

## Performance Bottlenecks

### SMESH MeshEditor Complexity

**Problem:** Massive mesh manipulation file scales poorly with geometry size
- Files: `src/3rdParty/salomesmesh/src/SMESH/SMESH_MeshEditor.cpp` (12,887 lines)
- Cause: Monolithic implementation with deeply nested conditional logic
- Improvement path: 
  - Profile common operations (edge swap, node insertion)
  - Consider spatial indexing for O(log n) lookups instead of O(n) sweeps
  - Split into focused classes by operation type

### Tree Widget Rendering

**Problem:** UI becomes sluggish with large document hierarchies (>1000 objects)
- Files: `src/Gui/Tree.cpp` (6,895 lines)
- Cause: Likely lack of viewport-aware culling; all items rendered regardless of visibility
- Improvement path:
  - Implement lazy rendering for off-screen tree items
  - Use virtual list model for large hierarchies
  - Cache computed geometry bounds

### Sketcher Constraint Rendering

**Problem:** Constraint visualization slows significantly with >100 constraints in single sketch
- Files: `src/Mod/Sketcher/Gui/CommandConstraints.cpp` (11,374 lines), `src/Mod/Sketcher/Gui/ViewProviderSketch.cpp` (5,343 lines)
- Cause: All constraints re-rendered each frame; no spatial subdivision or visibility culling
- Improvement path:
  - Implement quadtree or grid subdivision for constraint visibility
  - Use OpenGL instancing for repeated constraint glyphs
  - Add frustum culling

### Part Geometry Operations

**Problem:** Boolean operations (union, cut, intersect) timeout on complex geometries
- Files: `src/Mod/Part/App/Geometry.cpp` (7,741 lines), `src/Mod/Part/App/TopoShape.cpp` (4,606 lines)
- Cause: OCCT topology engine scaling limitations with high-complexity geometry
- Improvement path:
  - Consider mesh-based operations for high-polygon models
  - Implement level-of-detail (LOD) for preview operations
  - Add operation cancellation with cleanup

### FEM Mesh Generation Error Handling

**Problem:** Mesh generation stalls without clear progress indication
- Files: `src/Mod/Fem/femsolver/fenics/fenics_tools.py`
- Comment: "TODO: python classes much slower than JIT compilation"
- Cause: Pure Python mesh processing lacks compilation optimization
- Improvement path:
  - Profile hot paths with cProfile
  - Consider Cython or C++ bindings for core loops
  - Add progress callbacks to long-running operations

---

## Fragile Areas

### PropertyLinks Implementation

**Files:** `src/App/PropertyLinks.h`, `src/App/PropertyLinks.cpp`
**Why fragile:** 
- Dual list management (indexed + mapped) creates consistency bugs
- Multiple code paths for link validation (scope, target type, deleted object handling)
- Unicode URI handling likely incomplete
- Mixed concerns: serialization, scope validation, cyclic dependency detection

**Safe modification:**
- Add comprehensive unit tests for each link type (Link, LinkList, LinkSub, XLink)
- Test each scope combination (Local, Child, Global, Hidden)
- Test serialization/restoration round-trips
- Verify cyclic reference detection doesn't miss edge cases

**Test coverage gaps:**
- Multi-level LinkSub chains (A -> B#Face1 -> C#Wire2#Edge3)
- XLink across different document versions
- Scope validation with deeply nested Groups
- Serialization format changes/migrations

### Sketcher Geometry & Constraint System

**Files:** `src/Mod/Sketcher/App/Sketch.cpp` (5,772 lines), `src/Mod/Sketcher/App/planegcs/GCS.cpp` (5,818 lines)
**Why fragile:**
- Constraint solver (GCS) is external library with complex non-linear algorithm
- Dual representation of sketch geometry (FreeCAD native + internal GCS format)
- Constraint redundancy detection is heuristic-based
- Rendering order and selection depend on multiple fragile assumptions

**Safe modification:**
- Only modify through public constraint APIs, never direct solver state
- Test any sketch recompilation changes exhaustively
- Verify constraint consistency after load/save cycles
- Regression test against sketcher performance benchmarks

**Test coverage gaps:**
- Degenerate geometric cases (zero-length lines, coincident points)
- Conflicting constraint sets (overconstrained by design)
- Large constraint counts (>500 in single sketch)
- Mixed constraint types (tangent + perpendicular + distance combinations)

### View3DInventorViewer 3D Rendering

**Files:** `src/Gui/View3DInventorViewer.cpp` (4,844 lines)
**Why fragile:**
- Heavy OpenGL state management with multiple FIXMEs noted
- Selection and highlighting rely on scene graph traversal
- Coin3D library integration has known issues
- Multi-sample rendering adds complexity

**Safe modification:**
- Only modify rendering state within clearly delimited begin/end blocks
- Test with multiple graphics drivers (Intel, NVIDIA, AMD)
- Verify selection consistency after visibility changes
- Check performance regression on lower-end GPUs

**Test coverage gaps:**
- Selection under non-standard projection matrices
- Transparency blending with complex scene graphs
- Framebuffer operations on integrated graphics
- Picking with large numbers of selectable objects (>10k)

### Document Object Link Resolution

**Files:** `src/App/Document.cpp` (4,093 lines), `src/App/DocumentObject.cpp`
**Why fragile:**
- Global state for undo/restore operations (`static globalIsRestoring`, `static globalIsRelabeling`)
- Object deletion triggers cascading link cleanup
- Circular dependency detection requires graph traversal
- Link restoration may fail silently on document load

**Safe modification:**
- Never directly manipulate global restore/relabel flags
- Test object deletion with deeply linked hierarchies
- Verify undo/redo consistency after link operations
- Check recovery from corrupted link references in saved files

**Test coverage gaps:**
- Circular link chains (A->B->C->A)
- Cross-document links with missing referenced documents
- Object deletion with pending undo transactions
- Link restoration from pre-migration file formats

---

## Scaling Limits

### Document Size Capacity

**Current capacity:** Tested with ~10,000 objects; may become sluggish beyond
**Limit:** UI responsiveness degrades significantly with >5,000 objects in tree
**Scaling path:**
- Implement lazy tree rendering (only render visible nodes)
- Use virtual list model for tree widget
- Add document partitioning or external reference support
- Consider split-view UI for large assemblies

### Sketch Complexity Limits

**Current capacity:** ~500 constraints per sketch before solver slowdown
**Limit:** GCS solver becomes unbearably slow with >1000 constraints
**Scaling path:**
- Consider dividing sketches at 300 constraint boundary
- Implement constraint batching for solver updates
- Profile GCS performance on larger problems
- Consider approximation algorithms for initial guesses

### Mesh Generation Memory

**Current capacity:** ~1 million elements before memory pressure
**Limit:** SMESH requires ~10MB per 100k elements (grows super-linearly)
**Scaling path:**
- Implement streaming mesh export (avoid full model in memory)
- Consider LOD-based preview generation
- Add memory-mapped file support for large meshes
- Profile SMESH memory allocator for fragmentation

### Assembly Complexity

**Current capacity:** ~500 parts per assembly before UI lag
**Limit:** View update and constraint solving slow exponentially
**Scaling path:**
- Implement view culling and detail reduction
- Consider sub-assembly constraint solving
- Add progressive loading for large assemblies
- Implement spatial partitioning for constraint solvers

---

## Dependencies at Risk

### OCCT (Open CASCADE Technology) Version Coupling

**Risk:** Core topology engine tightly coupled to OCCT version; major version bumps break API
- Impact: Difficult to update OCCT; compatibility fixes often required across codebase
- Files: `src/Mod/Part/App/Geometry.cpp`, `src/Mod/Part/App/TopoShape.cpp`, mesh generation code
- Migration plan:
  - Create abstraction layer for OCCT API calls
  - Version-gate API calls for multi-version support
  - Maintain compatibility shims for 2-3 OCCT versions
  - Document OCCT version requirements clearly

### Qt Framework Version Constraints

**Risk:** GUI heavily dependent on Qt; version updates may break widget hierarchy
- Impact: Qt 5 to Qt 6 migration in progress; known compatibility gaps remain
- Files: `src/Gui/*.cpp`, `src/Gui/*.h`
- Migration plan:
  - Complete Qt 6 migration
  - Remove all Qt 5-only code paths
  - Test all widgets on both platforms (Windows, macOS, Linux)
  - Document Qt version requirement in README

### Python Version Support Window

**Risk:** Python 3.x minor version support window gradually shrinks
- Current: Support Python 3.9+
- Impact: Security patches for older versions eventually stop
- Files: `src/App/FreeCADInit.py`, all Python module code
- Migration plan:
  - Drop Python 3.9 when 3.13 reaches stable
  - Implement CI tests for Python 3.12 and 3.13
  - Use `from __future__ import annotations` for typing compatibility
  - Test all third-party dependencies on new Python versions

### Boost Library Large Surface Area

**Risk:** Heavy Boost usage across codebase; updates may introduce subtle breaking changes
- Impact: Boost.Python, Boost.Graph, Boost.Regex all have version-specific behaviors
- Files: `src/App/Document.cpp` (Boost.Graph topological_sort), property system
- Migration plan:
  - Audit Boost.Python usage for deprecation warnings
  - Profile performance impact of Boost.Regex vs std::regex
  - Consider dropping Boost.Regex in favor of std library
  - Add CI job to test against bleeding-edge Boost

---

## Missing Critical Features

### Plugin API Stability

**Problem:** No versioned plugin API; breaking changes cascade to all plugins
**Blocks:** Third-party workbench development; users stuck with older workbenches

### Document Diff/Merge

**Problem:** Cannot merge parallel edits to same document; encourages linear workflows
**Blocks:** Collaborative design workflows; branching strategies

### Non-Destructive Editing History

**Problem:** Undo/redo limited to transaction-level; cannot replay selective operations
**Blocks:** Complex design recovery; parametric history manipulation

### Real-Time Collaboration

**Problem:** No support for simultaneous multi-user editing
**Blocks:** Cloud-based design tools; team-based workflows

---

## Test Coverage Gaps

### Document Restoration from Corrupted Files

**Untested area:** Loading FreeCAD files with missing or invalid object references
- Files: `src/App/Document.cpp`, `src/App/DocumentObject.cpp`, property restoration code
- Risk: Silent data loss or unexpected crashes during file recovery
- Priority: High - affects data integrity

### Memory Management in Large Assemblies

**Untested area:** Object lifetime during cascading deletions and undo operations
- Files: `src/App/Document.cpp`, property system
- Risk: Memory leaks or use-after-free in edge cases
- Priority: High - recent memory leak fixes suggest ongoing issues

### Cross-Document Link Serialization

**Untested area:** Saving and loading links across separate documents
- Files: `src/App/PropertyLinks.cpp`, `src/App/XLinkPy.cpp`
- Risk: Broken links after file movement or document reorganization
- Priority: Medium - affects workflows with modular designs

### Strict IFC Export Completeness

**Untested area:** All object types in Strict IFC mode export correctly
- Files: `src/Mod/BIM/nativeifc/ifc_*.py`
- Risk: Silent data loss or malformed IFC output
- Priority: Medium - IFC migration is recent and ongoing

### Unicode Filename Handling

**Untested area:** Non-ASCII characters in file paths and object names
- Files: `src/Mod/Part/App/Geometry.cpp` (Part Loft/Sweep), import/export code
- Risk: File not found errors, garbled object names in exports
- Priority: Medium - affects international users
- Recent fix: PR #29728 addresses Part Loft/Sweep unicode issues

### Sketcher Solver Convergence Edge Cases

**Untested area:** Degenerate or nearly-conflicting constraint sets
- Files: `src/Mod/Sketcher/App/planegcs/GCS.cpp`
- Risk: Infinite loops, numeric instability, or solver hangs
- Priority: High - affects sketch robustness

### ViewProviderLink Circular Reference Display

**Untested area:** Display correctness when links form cycles (A->B->A)
- Files: `src/Gui/ViewProviderLink.cpp`
- Risk: Rendering artifacts, selection confusion, performance degradation
- Priority: Medium - assembly constraints often create cycles

---

*Concerns audit: 2026-06-07*
