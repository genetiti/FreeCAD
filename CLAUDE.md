<!-- GSD:project-start source:PROJECT.md -->

## Project

**FreeCAD: SolidWorks-Style UI**

A fork of FreeCAD that replaces the entire GUI with a SolidWorks-faithful interface — the CommandManager ribbon, the FeatureManager design tree, the sliding PropertyManager, and SolidWorks-style mouse/selection/navigation behavior. The goal is full *workflow parity*: not just a reskin, but an interface that looks, navigates, and behaves like SolidWorks, built on FreeCAD's parametric modeling engine underneath.

It is aimed at **SolidWorks users** so they can move to FreeCAD with effectively zero relearning of muscle memory.

**Core Value:** A SolidWorks user can open it and be immediately productive — it *looks* like SolidWorks, *navigates* like SolidWorks, and *behaves* like SolidWorks — with no FreeCAD tutorial required. If everything else is cut, this experience is the thing that must hold.

### Constraints

- **Tech stack**: C++20 / Qt 6.8 / Coin3D / OCCT 7.8 — all UI work must respect FreeCAD's App/Gui separation and the Workbench plugin pattern. Don't bypass Property change notifications or modify the document during recompute (see codebase ARCHITECTURE.md anti-patterns).
- **Fork baseline**: Tracks FreeCAD `main` (`1.2.0-dev`) — a moving target. The fork must be structured to absorb upstream changes without constant merge pain (favor additive/overriding Gui components over deep edits to shared code where feasible).
- **Platforms**: Must build and run on Windows, macOS, and Linux. Qt/Coin3D give cross-platform rendering, but SolidWorks-specific mouse/keyboard conventions must be reproduced consistently across all three.
- **Legal**: No verbatim SolidWorks proprietary assets. Recreated look-alike icons/themes only.
- **Single-threaded GUI**: Qt event loop is single-threaded; long operations must stay off the GUI thread (existing FreeCAD task-panel/worker pattern).

<!-- GSD:project-end -->

<!-- GSD:stack-start source:codebase/STACK.md -->

## Technology Stack

## Languages

- C++ (C++20, C++23 optional) - Core application, geometry engine, GUI, all modules
- Python 3.11.x - Python API, scripting, module system, internal tools
- CMake 3.22.0+ - Build system configuration
- JSON - Version configuration in `version.json`

## Runtime

- Cross-platform: Windows (MSVC toolchain), macOS (Clang), Linux (GCC/Clang)
- Conda (Pixi) as primary dependency manager - configured in `pixi.toml`
- CMake-based build system with pre-configured cmake presets in `CMakePresets.json`
- Pixi (Conda-based) - Multi-platform dependency management
- Pip - Secondary Python package manager for development dependencies

## Frameworks

- OpenCASCADE (OCCT) 7.8.x - Geometry kernel and CAD operations
- Qt 6.8.x (Qt5 support deprecated as of 2026-08) - GUI framework
- Coin3D - Open Inventor-compliant 3D scene representation
- PySide6/PySide2 - Python bindings for Qt
- Shiboken 6/2 - Code generator for Python bindings
- Pivy - Python bindings for Coin3D
- VTK (Visualization Toolkit) - Mesh processing, visualization
- Salome SMESH 7.7.1 - Advanced mesh generation and manipulation
- NETGEN - 3D mesh generation (optional, for FEM)
- IFCOpenShell - IFC file format support (BIM module)
- Xerces-C++ - XML processing for IFC and material definitions
- YAML-Cpp - YAML configuration parsing
- pybind11 - C++/Python bindings generator
- SWIG - Legacy bindings (still used in some modules)
- PyCXX - Utility library for Python/C++ integration
- Lark - Parser generator (used in BIM module for IFC schema parsing)
- Google Test (GTest) - C++ unit testing framework
- Boost 1.74+ - Core C++ libraries
- Eigen 3.3-5.x - Linear algebra library
- fmt - Modern formatting library
- ICU (International Components for Unicode) - Text internationalization
- OpenGL - Graphics rendering API (when BUILD_GUI enabled)
- Freetype - Font rendering and text rasterization
- Harfbuzz - Text shaping engine
- Zlib - Data compression
- DEFLATE (Clipper2) - Polygon clipping library
- KD-Tree - Spatial indexing (optional, BIM/Sketcher)
- TBB 2022.x (Intel Threading Building Blocks) - Parallelization
- ccache - Compiler cache for faster rebuilds (optional, enabled by default)
- Tracy - Frame profiler (optional)
- Points Cloud Library (PCL) - Point cloud processing
- OpenCV - Computer vision (listed in deps but not actively used in current build)
- NumPy 1.26.x - Numerical computing (Python)
- SciPy - Scientific computing (Python)
- Matplotlib - Plotting library (Python, for Plot workbench)
- SymPy - Symbolic math (Python)
- PyYAML - YAML parsing in Python
- defusedxml - Secure XML parsing (Python)
- lxml - XML/HTML processing (Python)
- Requests - HTTP client library (Python, used in tools)
- pycollada - COLLADA format support (Python)
- pythonocc-core - Python bindings for OCCT
- Doxygen - Documentation generation
- Graphviz - Graph visualization tools
- PlY - Lex/Yacc library (Python)

## Configuration

- CMake configuration entry point: `/Users/nguyenthuong/Repository/FreeCAD/CMakeLists.txt`
- Helper macros: `cMake/FreeCAD_Helpers/` directory
- Preset-based builds: `CMakePresets.json` with conda-linux-debug, conda-linux-release, conda-macos-debug, conda-macos-release, conda-windows-debug, conda-windows-release
- `version.json` - Single source of truth for version (major.minor.patch.suffix)
- Default: C++20 (required)
- Optional: C++23 via `BUILD_ENABLE_CXX_STD` CMake flag
- Feature testing: `cMake/ConfigureChecks.cmake`
- `src/config.h.cmake` - Platform detection and capability defines
- `src/FCConfig.h` - Global feature configuration
- `src/FCGlobal.h` - Exported symbols and version info
- `src/QtCore.h.cmake` - Qt compatibility layer (version abstraction)
- Precompiled headers (PCH): `FREECAD_USE_PCH` (ON by default on supported platforms)
- Compiler sanitizers: AddressSanitizer (ASan), LeakSanitizer (LSan), ThreadSanitizer (TSan), UndefinedBehaviorSanitizer (UBSan), MemorySanitizer (MSan)
- Dynamic linking: `BUILD_DYNAMIC_LINK_PYTHON` - Determines Python library linking strategy
- Ccache integration: `FREECAD_USE_CCACHE` (auto-detected)
- Warnings enabled: `-Wall -Wextra -Wno-write-strings` (GCC/Clang)
- Pedantic mode: `-Wpedantic` (Clang only)
- Color diagnostics: `-fdiagnostics-color` (GCC/Clang)
- Undefined symbol resolution: Platform-specific linker flags for dynamic linking safety

## Platform Requirements

- CMake 3.22.0+ (as of 2025-02)
- Git with submodule support
- C++20 compatible compiler (GCC 11.2+, Clang 14.0+, MSVC 2019+)
- Python 3.11+ interpreter and development headers
- Conda/Pixi environment configured with `pixi.toml`
- Linux x86-64 or AArch64 (ARM64)
- macOS 10.13+ (Intel or Apple Silicon)
- Windows 10/11
- Deployment target: macOS 10.13+, Windows 10+, Linux glibc 2.29+
- GUI requires X11/Wayland (Linux) or native compositor (macOS/Windows)
- OpenGL 3.3+ capable graphics hardware
- Approximately 4GB RAM minimum for building, 2GB+ recommended for runtime

<!-- GSD:stack-end -->

<!-- GSD:conventions-start source:CONVENTIONS.md -->

## Conventions

## Overview

## Naming Patterns

- Headers: `.h` (no `.hpp`)
- Implementation: `.cpp`
- Pattern: `PascalCase` (e.g., `Axis.h`, `Type.h`, `BaseClass.h`)
- Class names: `PascalCase` (e.g., `Axis`, `Placement`, `PropertyFloat`, `ApplicationDirectories`)
- Namespace names: `PascalCase` (e.g., `Base`, `App`, `Gui`)
- Enum names: `PascalCase` (e.g., `Base64ErrorHandling`)
- Template parameters: `UPPERCASE` or `PascalCase`
- Private members: `_lowerCamelCase` with leading underscore (e.g., `_base`, `_dir` in `Axis` class)
- Protected members: same as private
- Public accessors: `getXXX()` / `setXXX()` pattern (e.g., `getBase()`, `setBase()`, `getDirection()`, `setDirection()`)
- Free functions: `camelCase` (e.g., `appendVersionIfPossible`, `isDerivedFrom`)
- Method names: `camelCase` (e.g., `reverse()`, `reversed()`, `move()`)
- Modules: `snake_case.py` (e.g., `test_sync_version.py`, `test_creation.py`)
- Classes: `PascalCase` (e.g., `DraftCreation`, `ConsoleTestCase`, `UnitBasicCases`)
- Functions: `snake_case` (e.g., `compare`, `tu`, `ts`)
- Test classes: `PascalCase` with `Test` prefix or suffix (e.g., `UnitBasicCases`, `PropertyFloatTest`)
- C++ test files: Named after the component (e.g., `ApplicationDirectories.cpp`, `Property.cpp`)
- Python test files: `Test*.py` or `*Test.py` pattern (e.g., `TestDraft.py`, `TestDraftGui.py`, `UnitTests.py`)
- Test classes: `PascalCase` inheriting from `unittest.TestCase` (Python) or `::testing::Test` (C++)
- Test methods: `test_*` or `test*` (e.g., `testPrint()`, `testConversions()`)

## Code Style

- Configuration file: `.clang-format`
- Auto-formatting on pre-commit (see `.pre-commit-config.yaml`)
- Run formatting: pre-commit hooks apply clang-format automatically
- Based on LLVM style
- Indentation: 4 spaces (no tabs)
- Line length: 100 columns
- Pointer/Reference alignment: Left-aligned (e.g., `const Vector3d& getBase()`)
- Brace wrapping:
- No bin-packing of parameters (each parameter on new line if needed)
- Space before parens: control statements only
- Constructor initializer: break before comma
- Line length: 100 characters (configured in `.pre-commit-config.yaml`)
- Applied via pre-commit hooks

## Import Organization

- Order: Not enforced (SortIncludes: Never in `.clang-format`)
- Group structure: typically local includes, then third-party, then system
- Use: `#include "relative/path.h"` for local, `#include <system.h>` for system
- Standard library imports first
- Then third-party (e.g., `FreeCAD`, `unittest`)
- Then local imports
- Separated by blank lines

## Error Handling

- Exceptions: Uses C++ standard exceptions
- Example pattern: Throwing `std::runtime_error` for exceptional conditions (seen in `ApplicationDirectories.cpp`)
- NOLINT pragmas: Used to suppress clang-tidy warnings for specific sections
- Try-except blocks: Standard Python exception handling
- Pattern: `try:` / `except Exception:` for general exception catching
- Example from `BaseTests.py`:

## Logging and Console Output

- No centralized logging framework observed
- Console messages: Not found in base library patterns
- Uses FreeCAD Console methods for output
- Methods: `FreeCAD.Console.PrintMessage()`, `FreeCAD.Console.PrintError()`, `FreeCAD.Console.PrintWarning()`, `FreeCAD.Console.PrintLog()`
- Example from `BaseTests.py`:

## Comments and Documentation

- C++ comments: `//` for single line, `/* */` for blocks
- Header documentation: Doxygen-style comments above classes/functions
- Example from `Type.h`:
- Use `//` with two spaces before comment start
- Keep comments brief and meaningful
- C++: Doxygen-style comments with `\code`, `\endcode` blocks
- Python: Module and class docstrings describing purpose and usage
- Example from `TestDraft.py`:

## File Headers and Licensing

## Function Design

- Functions should be reasonably sized
- Clang-format enforces: `AllowShortFunctionsOnASingleLine: None` (one-liners not allowed)
- Example: `reverse()` in `Axis.h` is a full function definition
- No bin-packing: each parameter on its own line if needed
- Use const references for non-trivial types: `const Vector3d&`
- Example from `Axis.h`:
- Use simple types or const references
- Getters typically return const references or values
- Example: `const Vector3d& getBase() const`

## Module and Namespace Design

- Hierarchical: `Base`, `App`, `Gui`, `Mod::<ModuleName>`
- No indentation inside namespace (NamespaceIndentation: None)
- Example from `Type.h`:
- Test modules in `src/Mod/<Module>/` directory
- Module organization: `drafttests.test_creation`, `drafttests.test_modification`, etc.
- Imports in test orchestrators import from submodules
- Not typically used; imports explicit

## Code Quality Checks

- Trailing whitespace removal
- End-of-file fixer
- YAML validation
- Large file detection
- Line ending normalization (mixed line ending check)
- clang-format (C++ code)
- Black (Python code, 100-character line length)
- Version sync checks
- Files must pass pre-commit checks before committing
- Hooks are enforced in workflow via `sub_lint.yml`
- Handles code style checks for C++ and Python
- Does NOT duplicate pre-commit checks (formatting handled by pre-commit)
- Additional checks: Qt connection verification, pylint (optional)

## Contribution Guidelines

- PR title: brief (one line)
- PR body: detailed description with screenshots for UI changes
- Each commit: must compile cleanly with previous commits
- Checkpoint commits: should be squashed

## Special Patterns

- Use leading uppercase for parameters that shadow member variables: `Axis(const Vector3d& Orig, const Vector3d& Dir)`
- This convention distinguishes parameters from member variables
- For testing protected members, create subclass that exposes them
- Example from `ApplicationDirectories.cpp`:

<!-- GSD:conventions-end -->

<!-- GSD:architecture-start source:ARCHITECTURE.md -->

## Architecture

## System Overview

```text

```

## Component Responsibilities

| Component | Responsibility | File |
|-----------|----------------|------|
| App::Application | Core application instance, document management, module loading | `src/App/Application.h` |
| App::Document | Container for all objects in a project, transaction management, recompute orchestration | `src/App/Document.h` |
| App::DocumentObject | Base class for all parametric objects; property-driven, recomputable | `src/App/DocumentObject.h` |
| App::Property* | Typed value containers (Geometry, Links, Standard types); change notification | `src/App/Property*.h` |
| Gui::Application | GUI initialization, document/window management, command/macro system | `src/Gui/Application.h` |
| Gui::Document | GUI wrapper around App::Document; manages view providers, signals document changes | `src/Gui/Document.h` |
| ViewProvider | Graphical representation bridge; maps App::DocumentObject to Coin3D scene graph | `src/Gui/ViewProvider*.h` |
| Workbench | Pluggable UI context; defines menus, toolbars, dock windows per workbench | `src/Gui/Workbench.h` |
| Mod/* | Domain-specific modules (Part, PartDesign, Sketcher, etc.); extend App and Gui layers | `src/Mod/{Part,Sketcher,PartDesign,...}` |

## Pattern Overview

- **Document Object Model**: All design data resides in `App::Document` containing `App::DocumentObject` instances. The GUI mirrors this structure via `ViewProvider` objects.
- **Property-Driven**: Every `DocumentObject` exposes properties (Geometry, Links, Constraints, etc.). Property changes trigger observers, which may trigger recomputation.
- **Observable, Event-Driven**: Core uses signals/slots for document changes, selection, undo/redo, recompute events. GUI listens to App-layer signals.
- **Transaction/Undo System**: Document-level transactions group property changes; undo/redo replays transactions, not individual changes.
- **Lazy Recompute**: Objects marked as "touched" are recomputed on-demand when needed, not immediately on every property change.
- **Workbench Plugin Pattern**: UI elements (menus, toolbars, commands) are registered per workbench and activated/deactivated when switching workbenches.

## Layers

- Purpose: Cross-cutting utilities, math, persistence, plugin infrastructure
- Location: `src/Base/*`
- Contains: Vector/Matrix math, Exception handling, Console logging, Observer pattern, File I/O, Type system, Python bindings infrastructure
- Depends on: Standard C++ library, Python 3 (for bindings)
- Used by: App, Gui, all modules
- Purpose: Parametric object model, document management, dependency resolution, recomputation
- Location: `src/App/*`
- Contains: `Application` (singleton), `Document`, `DocumentObject` (base for all features), `Property` system (Property, PropertyContainer), `Transaction`, `Link` system, `FeaturePython` (Python-scriptable objects)
- Depends on: Base layer, XML serialization for .FCStd files, Python for scripting objects
- Used by: Gui, all workbench modules
- Purpose: Qt-based UI, 3D visualization via Coin3D, command dispatch, workbench management
- Location: `src/Gui/*`
- Contains: `Application` (GUI singleton), `Document` (GUI mirror), `ViewProvider` hierarchy (maps App objects to visuals), `MainWindow`, Command system, Task panels, Tree view, Property editor, Selection, Workbench registration
- Depends on: App layer, Qt 5/6, Coin3D (OpenGL 3D graphics), Pivy (Python bindings for Coin3D)
- Used by: Workbench modules (via their Gui/ subdirectories), user-facing code
- Purpose: Domain-specific functionality (modeling, meshing, CAM, BIM, etc.)
- Location: `src/Mod/{ModuleName}/{App,Gui,Resources,...}`
- Contains: Feature classes (e.g., `Sketcher::SketchObject`), Commands, Task panels, Python scripts
- Pattern: Each module has `App/` (features, algorithms) and `Gui/` (visualization, UI commands) subdirectories; `Init.py` (App-level) and `InitGui.py` (Gui-level) for registration
- Examples: `src/Mod/Part/`, `src/Mod/Sketcher/`, `src/Mod/PartDesign/`, `src/Mod/Mesh/`, `src/Mod/CAM/`
- Depends on: App, Gui, Base, often other modules (e.g., PartDesign depends on Part and Sketcher)

## Data Flow

### Primary Request Path: Feature Recomputation

### Secondary Flow: Document Save/Restore

### UI Command Execution

- **App state**: Managed by `App::Document` (objects, properties, transactions)
- **Gui state**: Managed by `Gui::Document` and `Gui::Application` (view providers, selection, active object)
- **Workbench state**: Workbench menus/toolbars shown/hidden on activation/deactivation

## Key Abstractions

- Purpose: Container and orchestrator for all parametric design objects
- Examples: `src/App/Document.h` (App layer), `src/Gui/Document.h` (GUI layer)
- Pattern: Observer pattern; emits signals for object creation, deletion, property change, recompute
- Purpose: Base class for all parametric features (sketches, parts, constraints, etc.)
- Examples: `src/App/DocumentObject.h`, `src/Mod/Sketcher/App/SketchObject.h`, `src/Mod/Part/App/FeaturePython.h`
- Pattern: Property-driven; implements `execute()` to compute output from input properties; virtual `onChanged()` for dependency tracking
- Purpose: Typed, observable value container; enables change notification and serialization
- Examples: `src/App/Property.h` (base), `src/App/PropertyStandard.h` (Int, Float, String), `src/App/PropertyGeo.h` (Geometry), `src/App/PropertyLinks.h` (DocumentObject links)
- Pattern: Observer via `PropertyContainer`; change triggers `onChanged(property)` callback; auto-serialized
- Purpose: GUI representation of an App::DocumentObject; bridges parametric model to 3D visualization
- Examples: `src/Gui/ViewProvider.h` (base), `src/Gui/ViewProviderGeometryObject.h`, workbench-specific subclasses in `Gui/` dirs
- Pattern: Listener on App::DocumentObject changes; maintains Coin3D `SoNode` scene graph; updates on property change signals
- Purpose: Pluggable UI context; defines which menus, toolbars, commands, and dock windows are visible
- Examples: `src/Gui/Workbench.h` (C++ base), `src/Mod/Part/InitGui.py::PartWorkbench` (Python subclass)
- Pattern: Registered with `Gui::Application` at startup; activated on user selection; `setupMenuBar()`, `setupToolBars()`, `setupDockWindows()` are called on activation
- Purpose: Scriptable DocumentObject; allows Python code to extend feature behavior
- Examples: `src/App/FeaturePython.h`
- Pattern: Wraps a Python class; calls Python methods for `execute()`, `onChanged()`, etc.

## Entry Points

- Location: `src/Main/MainGui.cpp` (function `main()`)
- Triggers: Process startup with `--gui` flag (default)
- Responsibilities: Qt initialization, App::Application singleton creation, Gui::Application singleton creation, module loading, main window display
- Execution order:
- Location: `src/Main/MainCmd.cpp`
- Triggers: Process startup with `--console` flag or when GUI disabled
- Responsibilities: App::Application initialization only, no Qt, Python interactive console or script execution
- Used by: Batch processing, server-side operations, CI/CD pipelines
- Location: `src/Mod/{ModuleName}/Init.py`
- Triggers: During `App::Application::init()` when module is loaded
- Responsibilities: Register feature classes, import types, set up Python API
- Example: `src/Mod/Part/Init.py` registers `Part::FeaturePython` class and import/export handlers
- Location: `src/Mod/{ModuleName}/InitGui.py`
- Triggers: During `Gui::Application::init()` or when workbench is activated
- Responsibilities: Register commands, create workbench instance, register UI elements
- Example: `src/Mod/Part/InitGui.py` creates `PartWorkbench()` and registers it via `Gui.addWorkbench()`
- Location: `src/Gui/Workbench.h` (virtual method `activated()`)
- Triggers: User selects workbench from workbench selector combo/tabs
- Responsibilities: Load lazy UI elements, initialize workbench-specific state
- Example: Sketcher workbench loads sketcher-specific commands, Part workbench loads Part commands

## Architectural Constraints

- **Threading:** Single-threaded event loop. App::Document recomputation can spawn worker threads for specific operations (e.g., mesh generation) via `canRecomputeOnWorker()`, but main document and property access is not thread-safe.
- **Global state:** `App::Application` and `Gui::Application` are singletons. Plugin factories (`WorkbenchFactory`, command registry) are global. No thread-local state.
- **Circular imports:** Python modules may have circular imports (e.g., workbench `InitGui.py` imports `PartGui` which imports back to Part module); resolved via lazy imports.
- **No copy semantics:** `DocumentObject`, `Document`, `Property` are not copyable; managed via reference counting (`App::Handle<>`) or raw pointers with external lifecycle management.
- **Coin3D scene graph lifetime:** Coin3D nodes (`SoNode`) are reference-counted and must be properly `ref()`/`unref()`'d to avoid memory leaks.

## Anti-Patterns

### Direct Property Access Without Observer Notification

```cpp

```

### Modifying Document During Recompute

```cpp

```

### Forgetting Property Observer Connections

```cpp

```

```cpp

```

### Blocking Operations in GUI Thread

```cpp

```

```cpp

```

## Error Handling

- **Property validation**: Properties validate input in `setValue()`. Invalid values are rejected or clamped.
- **Feature execution**: `execute()` returns `DocumentObjectExecReturn*` with error message if something fails. Null return = success.
- **Exceptions**: Used for severe errors (IO, corrupt data). Caught at document level.
- **GUI feedback**: Error messages displayed in UI via task panels or message boxes. Objects marked with `Error` status flag.

## Cross-Cutting Concerns

- Usage: `Base::Console().Log("message")`, `Base::Console().Warning("message")`

<!-- GSD:architecture-end -->

<!-- GSD:skills-start source:skills/ -->

## Project Skills

No project skills found. Add skills to any of: `.claude/skills/`, `.agents/skills/`, `.cursor/skills/`, `.github/skills/`, or `.codex/skills/` with a `SKILL.md` index file.
<!-- GSD:skills-end -->

<!-- GSD:workflow-start source:GSD defaults -->

## GSD Workflow Enforcement

Before using Edit, Write, or other file-changing tools, start work through a GSD command so planning artifacts and execution context stay in sync.

Use these entry points:

- `/gsd-quick` for small fixes, doc updates, and ad-hoc tasks
- `/gsd-debug` for investigation and bug fixing
- `/gsd-execute-phase` for planned phase work

Do not make direct repo edits outside a GSD workflow unless the user explicitly asks to bypass it.
<!-- GSD:workflow-end -->

<!-- GSD:profile-start -->

## Developer Profile

> Profile not yet configured. Run `/gsd-profile-user` to generate your developer profile.
> This section is managed by `generate-claude-profile` -- do not edit manually.
<!-- GSD:profile-end -->
