<!-- refreshed: 2026-06-07 -->
# Architecture

**Analysis Date:** 2026-06-07

## System Overview

FreeCAD is organized into a strict **App/Gui separation** with a **Document Object Model (DOM)** core and a **Workbench plugin architecture**. The three primary layers are:

```text
┌─────────────────────────────────────────────────────────────┐
│                      GUI Layer (Qt/Coin3D)                  │
│  Gui::Application, Gui::Document, ViewProvider, Workbench   │
│  `src/Gui/*`, `src/Mod/*/Gui/*`                             │
└────────────────┬────────────────┬──────────────────┬────────┘
                 │                │                  │
                 ▼                ▼                  ▼
┌──────────────────────────────────────────────────────────────┐
│              App Layer (C++ core, Property system)           │
│   App::Application, App::Document, DocumentObject            │
│   Property containers, Transaction system, Recompute engine  │
│   `src/App/*`, `src/Mod/*/App/*`                             │
└────────────────┬────────────────┬──────────────────┬────────┘
                 │                │                  │
                 └────────┬────────┴────────┬────────┘
                          ▼                ▼
         ┌─────────────────────────────────────────────┐
         │      Base Layer (Math, Persistence, I/O)    │
         │   Base::Vector3D, Base::Matrix4D, Observer  │
         │   Exceptions, Logging, Plugin system        │
         │   `src/Base/*`                              │
         └─────────────────────────────────────────────┘
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

**Overall:** Strict App/Gui separation with Document Object Model (DOM) and observable property system.

**Key Characteristics:**
- **Document Object Model**: All design data resides in `App::Document` containing `App::DocumentObject` instances. The GUI mirrors this structure via `ViewProvider` objects.
- **Property-Driven**: Every `DocumentObject` exposes properties (Geometry, Links, Constraints, etc.). Property changes trigger observers, which may trigger recomputation.
- **Observable, Event-Driven**: Core uses signals/slots for document changes, selection, undo/redo, recompute events. GUI listens to App-layer signals.
- **Transaction/Undo System**: Document-level transactions group property changes; undo/redo replays transactions, not individual changes.
- **Lazy Recompute**: Objects marked as "touched" are recomputed on-demand when needed, not immediately on every property change.
- **Workbench Plugin Pattern**: UI elements (menus, toolbars, commands) are registered per workbench and activated/deactivated when switching workbenches.

## Layers

**Base Layer:**
- Purpose: Cross-cutting utilities, math, persistence, plugin infrastructure
- Location: `src/Base/*`
- Contains: Vector/Matrix math, Exception handling, Console logging, Observer pattern, File I/O, Type system, Python bindings infrastructure
- Depends on: Standard C++ library, Python 3 (for bindings)
- Used by: App, Gui, all modules

**App Layer (Core):**
- Purpose: Parametric object model, document management, dependency resolution, recomputation
- Location: `src/App/*`
- Contains: `Application` (singleton), `Document`, `DocumentObject` (base for all features), `Property` system (Property, PropertyContainer), `Transaction`, `Link` system, `FeaturePython` (Python-scriptable objects)
- Depends on: Base layer, XML serialization for .FCStd files, Python for scripting objects
- Used by: Gui, all workbench modules

**Gui Layer:**
- Purpose: Qt-based UI, 3D visualization via Coin3D, command dispatch, workbench management
- Location: `src/Gui/*`
- Contains: `Application` (GUI singleton), `Document` (GUI mirror), `ViewProvider` hierarchy (maps App objects to visuals), `MainWindow`, Command system, Task panels, Tree view, Property editor, Selection, Workbench registration
- Depends on: App layer, Qt 5/6, Coin3D (OpenGL 3D graphics), Pivy (Python bindings for Coin3D)
- Used by: Workbench modules (via their Gui/ subdirectories), user-facing code

**Workbench Modules (Mod/):**
- Purpose: Domain-specific functionality (modeling, meshing, CAM, BIM, etc.)
- Location: `src/Mod/{ModuleName}/{App,Gui,Resources,...}`
- Contains: Feature classes (e.g., `Sketcher::SketchObject`), Commands, Task panels, Python scripts
- Pattern: Each module has `App/` (features, algorithms) and `Gui/` (visualization, UI commands) subdirectories; `Init.py` (App-level) and `InitGui.py` (Gui-level) for registration
- Examples: `src/Mod/Part/`, `src/Mod/Sketcher/`, `src/Mod/PartDesign/`, `src/Mod/Mesh/`, `src/Mod/CAM/`
- Depends on: App, Gui, Base, often other modules (e.g., PartDesign depends on Part and Sketcher)

## Data Flow

### Primary Request Path: Feature Recomputation

1. **User Action / Property Change** — User modifies a property via UI or script
   - File: `src/Gui/PropertyEditor.cpp` (UI property change) or Python API
   
2. **Property Changed Signal** — Property notifies its container (`DocumentObject`)
   - File: `src/App/Property.h` (Property base), `src/App/PropertyContainer.h`
   - Effect: `DocumentObject::StatusBits` is marked with `Touch` flag
   
3. **Dependency Marked** — `DocumentObject::onChanged()` callback fires, marks dependent objects
   - File: `src/App/DocumentObject.h` (virtual `onChanged()`)
   - Marks: All objects with property links to the changed object are touched
   
4. **Document Recompute Request** — User triggers `Document.recompute()` or automatic after transaction
   - File: `src/App/Document.h` (method `recompute()`)
   
5. **Topological Sort** — Document computes dependency order for objects to recompute
   - File: `src/App/Document.cpp` (method `_recomputeObject()`)
   - Logic: Builds a DAG of object dependencies via Property links
   
6. **Object Execution** — Each object's `execute()` method is called in dependency order
   - File: `src/App/DocumentObject.h` (virtual `execute()` and `mustExecute()`)
   - Return: `DocumentObjectExecReturn` (null = success, error object with message = failure)
   - Process: Objects compute their output geometry/data based on input properties
   
7. **ViewProvider Update** — Gui::Document listens to recompute signal, updates visualization
   - File: `src/Gui/Document.h` (slot `slotChangedObject()`)
   - Effect: `ViewProvider::update()` is called, regenerates Coin3D scene graph
   
8. **Scene Refresh** — Coin3D scene is re-rendered; 3D view updates
   - File: `src/Gui/View3DInventor.cpp`

### Secondary Flow: Document Save/Restore

1. **Save Document** — User saves `.FCStd` file
   - File: `src/App/Document.cpp` (method `save()`)
   - Process: Each `DocumentObject` serializes its properties via `PropertyContainer::Save()`
   - Format: ZIP with XML metadata + geometry data
   
2. **Restore Document** — User opens `.FCStd` file
   - File: `src/App/Document.cpp` (method `restore()`)
   - Process: Deserializes objects, rebuilds property links
   - Signals: `Document::signalBeginRestoreDocument`, `signalFinishRestoreDocument`
   - Action: Objects may be marked for recomputation if file format changed

### UI Command Execution

1. **Command Registered** — Commands are registered with `Gui::CommandManager` during workbench initialization
   - File: `src/Gui/CommandManager.h`, workbench `InitGui.py` (Python-based registration)
   
2. **Command Triggered** — User clicks menu item or presses shortcut; command dispatches
   - File: `src/Gui/Command.h` (virtual `activate()`)
   
3. **Task Panel Shown** (optional) — Multi-step commands open a task panel
   - File: `src/Gui/TaskView/TaskPanel.h`, workbench `Gui/` subdirectory
   
4. **Transaction Started** — Long-running operations open a transaction to group changes
   - File: `src/App/Document.h` (method `openTransaction()`)
   
5. **Document Undo/Redo** — Transaction applied or rolled back on undo
   - File: `src/App/Transaction.h`, `src/App/Document.h`

**State Management:**
- **App state**: Managed by `App::Document` (objects, properties, transactions)
- **Gui state**: Managed by `Gui::Document` and `Gui::Application` (view providers, selection, active object)
- **Workbench state**: Workbench menus/toolbars shown/hidden on activation/deactivation

## Key Abstractions

**Document:**
- Purpose: Container and orchestrator for all parametric design objects
- Examples: `src/App/Document.h` (App layer), `src/Gui/Document.h` (GUI layer)
- Pattern: Observer pattern; emits signals for object creation, deletion, property change, recompute

**DocumentObject:**
- Purpose: Base class for all parametric features (sketches, parts, constraints, etc.)
- Examples: `src/App/DocumentObject.h`, `src/Mod/Sketcher/App/SketchObject.h`, `src/Mod/Part/App/FeaturePython.h`
- Pattern: Property-driven; implements `execute()` to compute output from input properties; virtual `onChanged()` for dependency tracking

**Property:**
- Purpose: Typed, observable value container; enables change notification and serialization
- Examples: `src/App/Property.h` (base), `src/App/PropertyStandard.h` (Int, Float, String), `src/App/PropertyGeo.h` (Geometry), `src/App/PropertyLinks.h` (DocumentObject links)
- Pattern: Observer via `PropertyContainer`; change triggers `onChanged(property)` callback; auto-serialized

**ViewProvider:**
- Purpose: GUI representation of an App::DocumentObject; bridges parametric model to 3D visualization
- Examples: `src/Gui/ViewProvider.h` (base), `src/Gui/ViewProviderGeometryObject.h`, workbench-specific subclasses in `Gui/` dirs
- Pattern: Listener on App::DocumentObject changes; maintains Coin3D `SoNode` scene graph; updates on property change signals

**Workbench:**
- Purpose: Pluggable UI context; defines which menus, toolbars, commands, and dock windows are visible
- Examples: `src/Gui/Workbench.h` (C++ base), `src/Mod/Part/InitGui.py::PartWorkbench` (Python subclass)
- Pattern: Registered with `Gui::Application` at startup; activated on user selection; `setupMenuBar()`, `setupToolBars()`, `setupDockWindows()` are called on activation

**Feature (FeaturePython):**
- Purpose: Scriptable DocumentObject; allows Python code to extend feature behavior
- Examples: `src/App/FeaturePython.h`
- Pattern: Wraps a Python class; calls Python methods for `execute()`, `onChanged()`, etc.

## Entry Points

**Application Startup (GUI):**
- Location: `src/Main/MainGui.cpp` (function `main()`)
- Triggers: Process startup with `--gui` flag (default)
- Responsibilities: Qt initialization, App::Application singleton creation, Gui::Application singleton creation, module loading, main window display
- Execution order:
  1. Qt `QApplication` created
  2. `App::Application::init()` called — loads core modules and Python
  3. `Gui::Application::init()` called — initializes Qt UI, workbench system
  4. Workbenches registered via `InitGui.py` scripts in each `Mod/`
  5. Main window shown
  6. Event loop enters

**Application Startup (Console/Headless):**
- Location: `src/Main/MainCmd.cpp`
- Triggers: Process startup with `--console` flag or when GUI disabled
- Responsibilities: App::Application initialization only, no Qt, Python interactive console or script execution
- Used by: Batch processing, server-side operations, CI/CD pipelines

**Module Initialization (App Layer):**
- Location: `src/Mod/{ModuleName}/Init.py`
- Triggers: During `App::Application::init()` when module is loaded
- Responsibilities: Register feature classes, import types, set up Python API
- Example: `src/Mod/Part/Init.py` registers `Part::FeaturePython` class and import/export handlers

**Module Initialization (GUI Layer):**
- Location: `src/Mod/{ModuleName}/InitGui.py`
- Triggers: During `Gui::Application::init()` or when workbench is activated
- Responsibilities: Register commands, create workbench instance, register UI elements
- Example: `src/Mod/Part/InitGui.py` creates `PartWorkbench()` and registers it via `Gui.addWorkbench()`

**Workbench Activation:**
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

**What happens:** Code modifies `DocumentObject` member variables directly instead of setting Properties
```cpp
// BAD:
obj->Placement = newPlacement;  // Doesn't trigger Property change signal
// GOOD:
obj->getPropertyByName("Placement")->setValue(newPlacement);
// or for direct access:
static_cast<App::PropertyPlacement*>(obj->getPropertyByName("Placement"))->setValue(newPlacement);
```
**Why it's wrong:** Bypasses change notifications, breaks undo/redo, breaks dependent object tracking, breaks GUI synchronization
**Do this instead:** Always use Property setters or `Property::setValue()`. If you must modify internal state, call `touch()` to mark the object as needing recomputation.

### Modifying Document During Recompute

**What happens:** Code in `execute()` adds/removes objects from the document or modifies object properties
```cpp
// BAD:
DocumentObjectExecReturn* execute() override {
    doc->addObject(...);  // Modifying document during recomputation
    return new DocumentObjectExecReturn(...);
}
```
**Why it's wrong:** Recomputation order becomes undefined; circular dependencies possible; undo/redo state corrupts; concurrency issues if worker threads are enabled
**Do this instead:** Only modify input properties in `execute()`. If you need to create helper objects, do it in a separate command or via a Python setup script, not during execute.

### Forgetting Property Observer Connections

**What happens:** Code doesn't connect to property change signals when needed
```cpp
// BAD:
void MyViewProvider::attach(App::DocumentObject* obj) {
    // obj properties can change, but we never listen for it
}
```
**Why it's wrong:** ViewProvider doesn't update when model changes; UI gets out of sync with data
**Do this instead:** Use signal/slot connections in `attach()` and disconnect in `detach()`:
```cpp
connect(obj, SIGNAL(propertyChanged(...)), this, SLOT(onPropertyChanged(...)));
```

### Blocking Operations in GUI Thread

**What happens:** Long-running computations (mesh generation, CSG operations) run on the main Qt event loop thread
```cpp
// BAD (in a command):
solid = part.Cut(other);  // Locks UI for seconds
```
**Why it's wrong:** UI freezes, user can't cancel, no progress feedback
**Do this instead:** Use a `QThread` or task panel with progress updates:
```cpp
class MyTask : public TaskPanel {
    void onOkPressed() override {
        worker->computeInThread();  // Run on worker thread
        connect(worker, &Worker::finished, this, &MyTask::onFinished);
    }
};
```

## Error Handling

**Strategy:** Two-level error handling — property-level validation + feature execution exceptions

**Patterns:**
- **Property validation**: Properties validate input in `setValue()`. Invalid values are rejected or clamped.
- **Feature execution**: `execute()` returns `DocumentObjectExecReturn*` with error message if something fails. Null return = success.
- **Exceptions**: Used for severe errors (IO, corrupt data). Caught at document level.
- **GUI feedback**: Error messages displayed in UI via task panels or message boxes. Objects marked with `Error` status flag.

## Cross-Cutting Concerns

**Logging:** `Base::Console` singleton handles all logging. Levels: VERBOSE, LOG, WARNING, ERROR. Output routed to console window, log file, and Python console.
- Usage: `Base::Console().Log("message")`, `Base::Console().Warning("message")`

**Validation:** Properties validate on `setValue()`. Features validate inputs in `mustExecute()` and `execute()`. Document validates link integrity on restore.

**Authentication:** Not implemented in core. Authentication is for plug-ins (e.g., cloud addons) to handle via Python.

**Transactions:** All user-facing changes wrap in `Document::openTransaction()` / `closeTransaction()`. Undo/redo replays transactions via `Document::undo()` / `redo()`.

---

*Architecture analysis: 2026-06-07*
