# External Integrations

**Analysis Date:** 2026-06-07

## APIs & External Services

**Package Management:**
- Addon Manager - Native FreeCAD extension system
  - Module: `src/Mod/AddonManager/` (currently empty in repo, functionality in development)
  - Standard Python package format support
  - GitHub-based addon repository integration

**VCS & Build Integration:**
- GitHub - Source repository hosting
  - CI/CD: `.github/` workflows configured
  - Issue tracker: GitHub Issues
- Crowdin - Translation/localization service
  - Locale badge integrated in README
- Launchpad - Legacy VCS integration (LP importer tools in `src/Tools/SubWCRev.py`)
  - URL: `https://code.launchpad.net/~vcs-imports/freecad/trunk`

**Development Utilities:**
- Crowdin API - Automated translation updates via `src/Tools/updatecrowdin.py`
  - Uses `requests` HTTP library
- GitHub REST API - Repository statistics via `src/Tools/githubstats.py`
  - Uses `requests` HTTP library

## Data Storage

**Databases:**
- None - FreeCAD uses embedded document format (FCStd = ZIP container)
- `src/3rdParty/zipios/` - Internal ZIP library for document serialization
- Parameter storage: File-based (legacy .cfg format)

**File Storage:**
- Local filesystem only - All files stored as FCStd (compressed XML/geometry)
- Metadata: Stored within FCStd ZIP archive structure
- User config: `~/.FreeCAD/` (macOS/Linux) or `%APPDATA%\FreeCAD` (Windows)

**Caching:**
- In-memory caching of geometry kernels (OpenCASCADE instances)
- Render cache in Coin3D scene graph
- No external cache service

## File Format Integrations

**CAD Formats - Import/Export:**

**STEP (ISO 10303):**
- Implementation: `src/Mod/Import/App/ReaderStep.cpp`, `WriterStep.cpp`
- OpenCASCADE driver: `STEPCAFControl_Reader`, `STEPCAFControl_Writer`
- Python wrapper: `src/Mod/Import/InitGui.py` and `Init.py`
- Features: Color preservation via STEP AP 242
- Compressed variant: `src/Mod/Import/stepZ.py` - STEP files in ZIP format

**IGES (Initial Graphics Exchange Specification):**
- Implementation: `src/Mod/Import/App/ReaderIges.cpp`
- OpenCASCADE driver: `IGESCAFControl_Reader`, `IGESControl_Controller`
- Export: via Part module (traditional IGES without colors)
- Python registration: `src/Mod/Import/InitGui.py`

**IFC (Industry Foundation Classes):**
- Library: IFCOpenShell (Conda dependency: `ifcopenshell`)
- BIM Module: `src/Mod/BIM/` - Complete IFC support
  - IFC schema: `src/Mod/BIM/ArchIFCSchema.py`
  - IFC classes: `ArchIFC.py`, `ArchIFCView.py`
  - BIM objects with IFC properties: ArchProject, ArchFloor, ArchSite, ArchBuilding, etc.
- IFC read/write: Integrated in BIM workbench
- Data mapping: Python-based property system for IFC entities

**COLLADA (DAE):**
- Library: PyCollada (Conda dependency: `pycollada`)
- Format: XML-based 3D scene description
- Usage: Primarily import, used in asset pipeline

**DXF (AutoCAD Drawing Exchange Format):**
- Importer: `src/Mod/Draft/importDXF.py`
- Exporter: Draft module
- Remote fallback: GitHub-based DXF importer (`https://raw.githubusercontent.com/yorikvanhavre/Draft-dxf-importer`)

**SVG (Scalable Vector Graphics):**
- Importer: `src/Mod/Draft/importSVG.py`
- Parser: XML-based (Python standard library + namespace handling)
- Exporter: SVG output from Draft workbench
- Qt SVG Widget: For 2D visualization

**OBJ/STL/PLY (Mesh Formats):**
- Mesh Module: `src/Mod/Mesh/` - Multi-format mesh support
- ASCII and binary variants supported
- VTK integration for mesh processing

**OpenSCAD Format:**
- Module: `src/Mod/OpenSCAD/`
- Dual support: .scad file import and programmatic conversion

**Other Formats:**
- PDF: Via `src/Mod/TechDraw/` for technical drawing export
- SVG: Technical drawing export
- XAML: For GUI form export
- JT (STEP AP 203 variant): Optional JT reader module (`BUILD_JTREADER` flag)
- Airfoil DAT: `src/Mod/Draft/importAirfoilDAT.py` - Aircraft design support
- OCA/GCode: `src/Mod/Draft/importOCA.py` - CNC format support

## Authentication & Identity

**Auth Provider:**
- None - FreeCAD is fully offline-capable
- Optional cloud features require manual credential entry (no OAuth)
- GitHub addon registry: HTTP-based, no authentication required for downloads

## Monitoring & Observability

**Error Tracking:**
- None built-in - Relies on user bug reports
- Windows Minidump: `src/Main/MainGui.cpp` includes crash dump generation
  - Uses Windows Debug Help Library (`dbghelp.h`)
  - Minidump files saved for manual inspection

**Logs:**
- `src/Base/Console.h` - In-app console logging system
  - Console output: stderr/stdout
  - Log levels: Info, Warning, Error
  - Observers pattern for log routing
  - File logging support via parameter groups

**Stack Traces:**
- `src/Base/StackWalker.h` - Platform-specific stack walking
- GCC/Linux: Uses DWARF debugging info
- macOS: Native backtrace APIs
- Windows: Stack frame walking with symbol resolution

## CI/CD & Deployment

**Hosting:**
- GitHub - Primary repository and release distribution
- GitHub Releases - Precompiled binaries for Windows, macOS, Linux
- Distributed package managers:
  - Conda-Forge - Via `pixi.toml` integration
  - Linux package managers - Debian/Ubuntu, Fedora, Arch, etc.

**CI Pipeline:**
- GitHub Actions workflows: `.github/workflows/` directory
- Matrix builds: Multiple OS × Python versions × Qt versions
- Pre-commit hooks: `.pre-commit-config.yaml`
  - Code formatting, linting, spelling checks
- Packit integration: `.packit.yaml` for upstream CI automation

**Build Artifacts:**
- CMake-based build system (cross-platform)
- Conda environment lockfile: `pixi.lock`
- Build output: Portable binary bundles per platform
- LibPack (Windows): Pre-compiled dependency bundle available

**Documentation:**
- Doxygen: API documentation generation
  - Config: Various `.cmake` helpers
- Sphinx: User and developer documentation (wiki-hosted)

## VR/Spatial Integrations

**VR Support (Optional):**
- Oculus Rift Support (legacy):
  - Build flag: `BUILD_VR`
  - Find module: `cMake/FindRift.cmake`
  - Requires: Oculus SDK (LibOVR)
  - Status: Community-maintained, optional feature
- 3D Input Devices:
  - Spaceball/3DConnexion device support (Linux/macOS/Windows)
  - Configuration: `cMake/FreeCAD_Helpers/SetupSpaceball.cmake`

## Material & Property Systems

**Materials Database:**
- Material Module: `src/Mod/Material/`
- YAML-based material definitions
- External library support: Optional external material databases
- Build flag: `BUILD_MATERIAL_EXTERNAL` for custom material sources

## Robotics & CAM

**Robot Workbench:**
- Module: `src/Mod/Robot/`
- Trajectory planning and visualization
- KDL (Kinematics Dynamics Library) support
  - Configuration: `cMake/FreeCAD_Helpers/SetupKDL.cmake`
  - Can use external or bundled version

**CAM Workbench:**
- Module: `src/Mod/CAM/`
- CNC/Machining simulation
- CalculiX integration: `calculix` Conda dependency

**FEM Workbench:**
- Module: `src/Mod/Fem/`
- Finite Element Analysis
- Solvers supported:
  - CalculiX (Conda: `calculix`)
  - NETGEN mesh generator (optional, Conda: `netgen`)
  - Salome SMESH (integrated, Conda: `smesh`)

## Web & Network Features

**Help System:**
- Help Module: `src/Mod/Help/`
- Online/offline documentation links
- Embedded help browser

**Network Libraries:**
- Qt Network (QtNetwork) - HTTP/HTTPS support via Qt framework
- Requests library - Python-based HTTP for tools (`src/Tools/`)
- OpenSSL - TLS/SSL support (Conda: `openssl`)
- No custom web server or REST API

**Crowdin Integration:**
- Translation platform for internationalization
- Automated pull requests for translation updates
- Badge/status display in documentation

## Python Ecosystem Integration

**Standard Library Usage:**
- `urllib.parse`, `urllib.request` - HTTP operations in tools
- `xml.etree` - XML parsing (defusedxml wrapper for security)
- Standard logging - Console output system

**Third-Party Python Packages:**
- NumPy/SciPy/Matplotlib - Scientific computing stack
- SymPy - Symbolic mathematics
- Requests - HTTP client
- PyYAML - Configuration parsing
- lxml - XML processing
- pandas - Data analysis (optional)
- six - Python 2/3 compatibility (legacy)
- blinker - Signal/slot system (event dispatcher)
- Debugpy - Python debugging support (dev dependency)
- Pyright - Python type checker (dev dependency)
- Pre-commit - Git hook framework (dev dependency)

## Source Code Analysis & Linting

**Code Quality Tools:**
- `.clang-format` - C++ formatting rules
- `.clang-tidy` - Static analysis configuration (29KB config file)
- `.pylintrc` - Python linting rules (20KB config file)
- Pyright - Python type checking with strict configuration
- CMake linting via custom helpers

## Internationalization

**Translation Services:**
- Crowdin - Web-based translation platform
  - URL: `https://crowdin.com/project/freecad`
  - Automated sync via GitHub Actions
- Locale system: Qt translation framework
- Supported: 40+ languages via Crowdin community

## External Compute/Solver Integration

**Mesh Generation:**
- NETGEN 6.x+ (Conda: `netgen`) - 3D automatic meshing
- Salome SMESH 7.7.1 (Conda: `smesh`) - Industrial-strength meshing
- PCL (Point Cloud Library) - Point cloud processing and features

**Symbolic Computation:**
- SymPy (Conda: `sympy`) - Symbolic math for Spreadsheet workbench
- PyYAML parsing for material definitions

**Scientific Computing:**
- NumPy/SciPy - Array operations and algorithms
- Matplotlib - Plotting and visualization

## Desktop Integration

**Launcher/Desktop Files:**
- Data directory: `src/XDGData/` - XDG desktop integration (Linux)
- Icons: `src/Gui/Icons/` - Cross-platform icon resources
- OS-specific launchers:
  - macOS: `src/MacAppBundle/` - Bundle structure
  - Windows: `src/Main/freecad.rc.cmake` - Resource definitions

## External File Associations

**File Type Handlers:**
- `.FCStd` - Native FreeCAD document (ZIP container with XML)
- Import associations: STEP, IGES, DXF, SVG, OBJ, STL, etc.
- Export associations: STEP, IGES, PDF, SVG, etc.

---

*Integration audit: 2026-06-07*
