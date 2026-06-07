# Codebase Structure

**Analysis Date:** 2026-06-07

## Directory Layout

```
/Users/nguyenthuong/Repository/FreeCAD/
├── src/                      # Main source directory
│   ├── App/                  # Core application, document, document object, property system
│   ├── Gui/                  # Qt-based GUI, 3D visualization, workbench framework
│   ├── Base/                 # Cross-cutting utilities: math, exceptions, logging, persistence
│   ├── Main/                 # Application entry points (GUI and console modes)
│   ├── Mod/                  # Workbench modules (parametric features, domain logic)
│   ├── Ext/                  # External Qt extensions and utilities
│   ├── Tools/                # Build and development tools
│   ├── 3rdParty/             # Third-party dependencies (header-only or vendored)
│   ├── Build/                # Build version info and configuration
│   ├── Doc/                  # Documentation configuration (Doxygen)
│   └── MacAppBundle/         # macOS-specific app bundle configuration
├── cMake/                    # CMake build system scripts
├── tests/                    # Test suite (unit and visual tests)
├── tools/                    # Development and utility scripts (linting, profiling)
├── contrib/                  # Community contributions (IDE configs, debugger configs)
├── data/                     # Example files, test data, resource files
├── package/                  # Packaging scripts (Windows installer, rpm, deb, etc.)
└── CMakeLists.txt            # Root CMake configuration
```

## Directory Purposes

**`src/App/`:**
- Purpose: Core parametric object model and document management
- Contains: `Application`, `Document`, `DocumentObject`, `Property` and subclasses, `FeaturePython`, transaction system, recompute engine
- Key files: `Application.h`, `Document.h`, `DocumentObject.h`, `Property*.h`, `PropertyContainer.h`, `Transaction.h`, `Link.h`
- Dependencies: Base, Python, XML libraries (Expat)
- Used by: All GUI code, all modules

**`src/Gui/`:**
- Purpose: Qt-based user interface, 3D visualization via Coin3D, command/action dispatch, workbench management
- Contains: `Application`, `Document` (GUI mirror), `ViewProvider` hierarchy, `Command` framework, `MainWindow`, tree view, property editor, task panels, workbench factory
- Key files: `Application.h`, `Document.h`, `ViewProvider*.h`, `Command.h`, `Workbench.h`, `WorkbenchFactory.h`, `MainWindow.h`
- Dependencies: App, Base, Qt 5/6, Coin3D, Pivy
- Subdirs: `TaskView/` (task panel framework), `DAGView/` (dependency graph visualization), `SelectionView/` (selection handling), `Dialogs/` (common dialogs)

**`src/Base/`:**
- Purpose: Foundational cross-cutting utilities
- Contains: Math libraries (Vector3D, Matrix4D, Placement, Quaternion), Exception types, Console logging, Persistence I/O, Observer pattern, Plugin Factory, Type system, Python bindings infrastructure
- Key files: `Vector3D.h`, `Matrix4D.h`, `Placement.h`, `Exception.h`, `Console.h`, `Observer.h`, `Factory.h`, `Type.h`, `Persistence.h`
- Dependencies: Standard C++, Python 3
- Used by: App, Gui, Base itself, all modules

**`src/Main/`:**
- Purpose: Application entry points and initialization
- Contains: `main()` functions for GUI mode (`MainGui.cpp`) and console mode (`MainCmd.cpp`), startup logging, exception handling
- Key files: `MainGui.cpp`, `MainCmd.cpp`, `MainPy.cpp` (Python embedding)
- Entry points: `FreeCAD` executable (GUI), `FreeCADCmd` executable (console)

**`src/Mod/`:**
- Purpose: Pluggable workbench modules providing domain-specific functionality
- Contains: ~30 subdirectories, one per workbench (Part, PartDesign, Sketcher, Mesh, CAM, BIM, Fem, TechDraw, etc.)
- Pattern: Each module has:
  - `App/` — Feature classes (C++), algorithms, domain-specific property types
  - `Gui/` — ViewProviders (visualization), Commands (user actions), Task panels (multi-step UI)
  - `Init.py` — App-level module initialization (feature registration)
  - `InitGui.py` — GUI-level workbench initialization (command registration, workbench setup)
  - `Resources/` — Icons, translation files
  - `Tests/` or `Testing/` — Module-specific tests

**`src/Ext/`:**
- Purpose: Qt extensions and custom widgets
- Contains: Custom dock windows, tree widgets, action frameworks, styling utilities
- Not critical to core architecture; mostly UI convenience

**`src/Tools/`:**
- Purpose: Build-time and development tools
- Contains: Python code generation tools, script utilities
- Not part of the runtime

**`src/3rdParty/`:**
- Purpose: Vendored third-party header-only libraries
- Contains: Boost headers, expected.hpp, fastsignals (signal/slot library)
- Note: Larger dependencies (Qt, Coin3D, Python) are installed separately, not vendored

**`src/Build/`:**
- Purpose: Build version and branding information
- Contains: `Version.h.in` (CMake template) with version numbers, copyright year
- Generated: CMake processes `Version.h.in` to create actual `Version.h` at build time

**`src/Doc/`:**
- Purpose: Doxygen configuration for API documentation
- Contains: `doxygen.conf` template and customization files

**`src/MacAppBundle/`:**
- Purpose: macOS-specific application bundle metadata
- Contains: `Info.plist` template, icon files, code signing configuration

## Key File Locations

**Entry Points:**

- `src/Main/MainGui.cpp` — GUI application entry point; contains `main()` function for FreeCAD executable
- `src/Main/MainCmd.cpp` — Console application entry point; contains `main()` function for FreeCADCmd executable
- `src/Main/MainPy.cpp` — Python embedding initialization

**Core Singletons & Initialization:**

- `src/App/Application.h/.cpp` — App::Application singleton; module loading, document management
- `src/Gui/Application.h/.cpp` — Gui::Application singleton; window management, command dispatch, workbench activation
- `src/Base/Interpreter.h/.cpp` — Python interpreter initialization and API

**Configuration:**

- `CMakeLists.txt` (root) — Build configuration, compiler flags, library dependencies
- `cMake/FreeCAD_Helpers/` — CMake helper modules
- `src/Gui/PreferencePages/` — Preference/settings panels (accessible via Edit menu → Preferences)
- `src/App/Application.cpp` — Config file parsing and defaults

**Document Model:**

- `src/App/Document.h/.cpp` — App::Document container, object management, recompute orchestration
- `src/App/DocumentObject.h/.cpp` — Base class for all parametric features
- `src/App/PropertyContainer.h/.cpp` — Base class for objects with properties
- `src/App/Property.h/.cpp` — Base property class
- `src/App/Property*.h` — Specific property types (PropertyInt, PropertyString, PropertyGeometry, PropertyLink, etc.)

**GUI Components:**

- `src/Gui/Document.h/.cpp` — GUI-level document wrapper; manages ViewProviders, signals GUI updates
- `src/Gui/ViewProvider.h/.cpp` — Base class for graphical representation of App::DocumentObject
- `src/Gui/ViewProvider*.h` — Specific ViewProvider subclasses (e.g., ViewProviderGeometryObject, ViewProviderFeature)
- `src/Gui/MainWindow.h/.cpp` — Main application window; docks, menus, toolbars
- `src/Gui/Workbench.h/.cpp` — Workbench base class; defines menu/toolbar/dock setup
- `src/Gui/WorkbenchFactory.h` — Factory for registering and instantiating workbenches
- `src/Gui/Command.h` — Command base class for user actions
- `src/Gui/CommandManager.h` — Command registry and dispatch
- `src/Gui/TaskView/TaskPanel.h` — Multi-step task panels (wizards, property forms)
- `src/Gui/SelectionView.h` — Selection management and tree view

**Transaction & Undo/Redo:**

- `src/App/Document.h` — Methods `openTransaction()`, `closeTransaction()`, `undo()`, `redo()`
- `src/App/Transaction.h` — Transaction object; records document changes
- `src/App/TransactionDefs.h` — Transaction-related enums and constants

**Module Registration & Loading:**

- `src/Mod/{ModuleName}/Init.py` — App-level initialization; registers feature classes
- `src/Mod/{ModuleName}/InitGui.py` — GUI-level initialization; creates Workbench, registers commands
- `src/Gui/Workbench.h` / `src/Gui/WorkbenchFactory.h` — Workbench plugin system
- `src/Base/Factory.h` — Generic factory pattern for plugin registration

**Tests:**

- `tests/src/` — C++ unit tests (CMake-based, runs via `ctest`)
- `tests/visual/` — Visual/regression tests (Python-based)
- `src/Mod/{ModuleName}/Test*.py` — Module-specific Python tests

## Naming Conventions

**Files:**

- C++ headers: `.h` (public) or `.hpp` (internal/template-heavy)
- C++ implementation: `.cpp`
- Python scripts: `.py`
- CMake: `CMakeLists.txt` or `.cmake` files
- UI definitions: `.ui` (Qt Designer XML), compiled to C++ by `uic`
- Documentation: `.dox` (Doxygen markdown)
- Executables: `FreeCAD` (GUI), `FreeCADCmd` (console)

**C++ Classes/Namespaces:**

- App-level classes: `App::ClassName` (e.g., `App::Document`, `App::DocumentObject`)
- GUI-level classes: `Gui::ClassName` (e.g., `Gui::Application`, `Gui::ViewProvider`)
- Base utilities: `Base::ClassName` (e.g., `Base::Vector3D`, `Base::Console`)
- Module-specific: `{ModuleName}::ClassName` (e.g., `Sketcher::SketchObject`, `Part::FeaturePython`)

**Python Classes:**

- Workbench classes: `{ModuleName}Workbench` subclass of `Gui.Workbench` (e.g., `PartWorkbench`, `SketcherWorkbench`)
- Feature classes: `FreeCADGui.ViewProvider*` or module-specific subclasses
- Commands: Not directly exposed; registered via `Gui.addCommand()` in `InitGui.py`

**Properties:**

- Property names in DocumentObject: CamelCase (e.g., `Placement`, `Label`, `Visibility`)
- Property internal members: `m_propertyName` or direct property object in class declaration via macro `PROPERTY_HEADER()`

**Functions/Methods:**

- C++ naming: `camelCase()` for public API, `_privateMethod()` for internal
- Python naming: `snake_case()` or `camelCase()` (consistency with Qt bindings)
- Callback methods: `on{EventName}()` (e.g., `onChanged()`, `onPropertyChanged()`)

**Directories:**

- Core modules: PascalCase (e.g., `App/`, `Gui/`, `Base/`)
- Feature modules: PascalCase (e.g., `Part/`, `Sketcher/`, `PartDesign/`)
- Utilities: lowercase or PascalCase consistently
- Test directories: `Test*` or `*Tests` suffix (e.g., `SketcherTests/`)

## Where to Add New Code

**New Feature (e.g., a new solid modeling operation):**

1. **Primary code location:**
   - Create `src/Mod/Part/App/FeatureBoxWithHole.h/.cpp` (if extending Part workbench)
   - Inherit from `Part::Feature` or `App::GeoFeature`
   - Implement `execute()` method to compute geometry
   - Add properties via `PROPERTY_HEADER()` macro

2. **Tests:**
   - `src/Mod/Part/Test*.py` — Python test file
   - Run: `cd build && python -m pytest ../src/Mod/Part/TestPartApp.py` or `FreeCADCmd --run-tests`

3. **GUI Visualization:**
   - Create `src/Mod/Part/Gui/ViewProviderFeatureBoxWithHole.h/.cpp`
   - Inherit from `Gui::ViewProviderGeometryObject`
   - Implement `attach()` to create Coin3D scene graph
   - Implement `update()` to refresh when properties change

4. **User Command:**
   - Create `src/Mod/Part/Gui/CommandPartBoxWithHole.cpp`
   - Inherit from `Gui::Command`
   - Implement `activate()` to create object or show task panel
   - Register in `src/Mod/Part/InitGui.py` via `Gui.addCommand()`

5. **Workbench Integration:**
   - Add command to toolbar/menu in `src/Mod/Part/InitGui.py` (setup in `PartWorkbench`)

**New Workbench Module:**

1. Create directory: `src/Mod/MyNewModule/`
2. Create subdirs: `App/`, `Gui/`, `Resources/icons/`
3. Create `Init.py`:
   ```python
   import FreeCAD
   FreeCAD.addImportType("My Format (*.myn)", "MyNewModule")
   # Register feature classes via Python or C++ bindings
   ```
4. Create `InitGui.py`:
   ```python
   import FreeCADGui as Gui
   class MyNewWorkbench(Gui.Workbench):
       def Initialize(self):
           import MyNewModuleGui
       def setupMenuBar(self):
           # Return Menu tree
       def setupToolBars(self):
           # Return ToolBar tree
   Gui.addWorkbench(MyNewWorkbench())
   ```
5. Feature classes in `App/`: Inherit from `App::DocumentObject` or `App::GeoFeature`
6. Commands in `Gui/`: Inherit from `Gui::Command`
7. Workbench setup in `InitGui.py`
8. Update root `CMakeLists.txt` to include your module's `CMakeLists.txt`

**Shared Utilities:**

- Cross-module helpers: `src/Base/` (math, exceptions, persistence)
- Qt/GUI utilities: `src/Ext/` (custom widgets, UI helpers)
- App-layer utilities: `src/App/` (e.g., property factories, link helpers)

**Adding Properties to a Feature:**

1. Declare in class header using macro:
   ```cpp
   class MyFeature : public App::GeoFeature {
       PROPERTY_HEADER(MyFeature);
   public:
       App::PropertyFloat Radius = {0.0};
       App::PropertyString Name = {"DefaultName"};
   };
   ```
2. Register in implementation (`.cpp`) via `PROPERTY_SOURCE_ABSTRACT()`
3. Handle changes in `onChanged()` callback if needed
4. Recompute in `execute()` using property values

**Adding a Task Panel (Multi-step Dialog):**

1. Create `src/Mod/{Module}/Gui/TaskPanel{Name}.h/.cpp`
2. Inherit from `Gui::TaskView::TaskPanel`
3. Implement `getStandardButtons()`, `onOkPressed()`, `onCancelPressed()`
4. In your command's `activate()`, create panel and show via `Gui::Control().showTaskView(panel)`

## Special Directories

**`src/Gui/TaskView/`:**
- Purpose: Framework for multi-step task panels (wizards)
- Generated: No (source code)
- Committed: Yes
- Key files: `TaskPanel.h` (base), `TaskWatcher.h` (integrates with main window)

**`src/Gui/DAGView/`:**
- Purpose: Dependency graph visualization (shows object dependencies)
- Generated: No
- Committed: Yes

**`build/` (at repo root or elsewhere):**
- Purpose: CMake build output
- Generated: Yes (created by `cmake` command)
- Committed: No (add to `.gitignore`)
- Contents: `.o` files, libraries, executables, CMake cache

**`src/Mod/{Module}/Resources/`:**
- Purpose: Icons, translations, resource files per module
- Generated: Partially (translations may be compiled)
- Committed: Yes (sources)
- Contents: `.svg`, `.png` icons; `.ts` translation files

**`src/App/` Header Files Naming Pattern:**

- `Document.h` — The Document class
- `DocumentObject.h` — The DocumentObject base class
- `Property.h` — Base Property class
- `PropertyStandard.h` — Int, Float, String property types
- `PropertyGeo.h` — Geometry-related properties (Placement, Vector, etc.)
- `PropertyLinks.h` — Link properties (references to other objects)
- `PropertyContainer.h` — Base for objects with properties
- `PropertyExpressionEngine.h` — Expression (formula) support in properties

---

*Structure analysis: 2026-06-07*
