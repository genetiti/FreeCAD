# Technology Stack

**Analysis Date:** 2026-06-07

## Languages

**Primary:**
- C++ (C++20, C++23 optional) - Core application, geometry engine, GUI, all modules
  - Minimum compiler versions: GCC 11.2+, Clang 14.0+
- Python 3.11.x - Python API, scripting, module system, internal tools
  - Minimum: Python 3.10+, configured in `cMake/FreeCAD_Helpers/SetupPython.cmake`

**Secondary:**
- CMake 3.22.0+ - Build system configuration
- JSON - Version configuration in `version.json`

## Runtime

**Environment:**
- Cross-platform: Windows (MSVC toolchain), macOS (Clang), Linux (GCC/Clang)
- Conda (Pixi) as primary dependency manager - configured in `pixi.toml`
- CMake-based build system with pre-configured cmake presets in `CMakePresets.json`

**Package Manager:**
- Pixi (Conda-based) - Multi-platform dependency management
  - Lockfile: `pixi.lock` (877KB, comprehensive lock state)
  - Supports platforms: linux-64, linux-aarch64, osx-64, osx-arm64, win-64
- Pip - Secondary Python package manager for development dependencies

## Frameworks

**Core:**
- OpenCASCADE (OCCT) 7.8.x - Geometry kernel and CAD operations
  - Configuration: `cMake/FreeCAD_Helpers/SetupOpenCasCade.cmake`
  - Provides: STEP/IGES readers/writers, shape modeling, topology
- Qt 6.8.x (Qt5 support deprecated as of 2026-08) - GUI framework
  - Core components: QtCore, QtGui, QtWidgets, QtNetwork, QtXml, QtSvg, QtOpenGL, QtPrintSupport
  - Configuration: `cMake/FreeCAD_Helpers/SetupQt.cmake`
  - Automatic MOC/UIC enabled at build time
- Coin3D - Open Inventor-compliant 3D scene representation
  - 3D visualization and rendering pipeline
  - Configuration: `cMake/FreeCAD_Helpers/SetupCoin3D.cmake`

**GUI Bindings:**
- PySide6/PySide2 - Python bindings for Qt
  - Configuration: `cMake/FreeCAD_Helpers/SetupShibokenAndPyside.cmake`
- Shiboken 6/2 - Code generator for Python bindings
- Pivy - Python bindings for Coin3D

**Geometry & Mesh:**
- VTK (Visualization Toolkit) - Mesh processing, visualization
  - Used by: SMESH, FEM module, points/mesh visualization
  - Version compatibility: VTK 7+, VTK 9 preferred
- Salome SMESH 7.7.1 - Advanced mesh generation and manipulation
  - Can use internal bundled version or external system installation
  - Configuration: `cMake/FreeCAD_Helpers/SetupSalomeSMESH.cmake`
- NETGEN - 3D mesh generation (optional, for FEM)
  - Configuration: Conditional build via `BUILD_FEM_NETGEN` flag

**Data & Formats:**
- IFCOpenShell - IFC file format support (BIM module)
  - Conda dependency: `ifcopenshell`
- Xerces-C++ - XML processing for IFC and material definitions
  - Configuration: `cMake/FreeCAD_Helpers/SetupXercesC.cmake`
- YAML-Cpp - YAML configuration parsing
  - Configuration: `cMake/FreeCAD_Helpers/SetupYamlCpp.cmake`

**Python & Scripting:**
- pybind11 - C++/Python bindings generator
  - Configuration: `cMake/FreeCAD_Helpers/SetupPybind11.cmake`
- SWIG - Legacy bindings (still used in some modules)
  - Configuration: `cMake/FreeCAD_Helpers/SetupSwig.cmake`
- PyCXX - Utility library for Python/C++ integration
- Lark - Parser generator (used in BIM module for IFC schema parsing)
  - Configuration: `cMake/FreeCAD_Helpers/SetupLark.cmake`

**Testing:**
- Google Test (GTest) - C++ unit testing framework
  - Enabled via `ENABLE_DEVELOPER_TESTS` CMake option
  - Test runner: `ctest` via CMake

**Build & Development:**
- Boost 1.74+ - Core C++ libraries
  - Components: program_options, regex, thread, date_time
  - Configuration: `cMake/FreeCAD_Helpers/SetupBoost.cmake`
- Eigen 3.3-5.x - Linear algebra library
  - Configuration: `cMake/FreeCAD_Helpers/SetupEigen.cmake`
- fmt - Modern formatting library
- ICU (International Components for Unicode) - Text internationalization
- OpenGL - Graphics rendering API (when BUILD_GUI enabled)
  - Configuration: `cMake/FreeCAD_Helpers/SetupOpenGL.cmake`
- Freetype - Font rendering and text rasterization
- Harfbuzz - Text shaping engine
- Zlib - Data compression
- DEFLATE (Clipper2) - Polygon clipping library
  - Configuration: `cMake/FreeCAD_Helpers/SetupClipper2.cmake`
- KD-Tree - Spatial indexing (optional, BIM/Sketcher)
  - Configuration: `cMake/FreeCAD_Helpers/SetupKDTree.cmake`

**Optimization & Profiling:**
- TBB 2022.x (Intel Threading Building Blocks) - Parallelization
  - Conda dependency pinned: `tbb-devel >=2022,<2023`
- ccache - Compiler cache for faster rebuilds (optional, enabled by default)
  - Controlled via `FREECAD_USE_CCACHE` flag
- Tracy - Frame profiler (optional)
  - Build flag: `BUILD_TRACY_FRAME_PROFILER`

**Third-party Libraries:**
- Points Cloud Library (PCL) - Point cloud processing
  - Optional build: `FREECAD_USE_PCL`
  - Configuration: `cMake/FreeCAD_Helpers/SetupPCL.cmake`
  - Components: common, kdtree, features, surface, io, filters, segmentation, sample_consensus
- OpenCV - Computer vision (listed in deps but not actively used in current build)
- NumPy 1.26.x - Numerical computing (Python)
- SciPy - Scientific computing (Python)
- Matplotlib - Plotting library (Python, for Plot workbench)
  - Configuration: `cMake/FreeCAD_Helpers/SetupMatplotlib.cmake`
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

**Build System:**
- CMake configuration entry point: `/Users/nguyenthuong/Repository/FreeCAD/CMakeLists.txt`
- Helper macros: `cMake/FreeCAD_Helpers/` directory
- Preset-based builds: `CMakePresets.json` with conda-linux-debug, conda-linux-release, conda-macos-debug, conda-macos-release, conda-windows-debug, conda-windows-release

**Version Configuration:**
- `version.json` - Single source of truth for version (major.minor.patch.suffix)
  - Current: 1.2.0-dev
  - Synced to build artifacts via `src/Tools/sync_version.py`

**C++ Standard:**
- Default: C++20 (required)
- Optional: C++23 via `BUILD_ENABLE_CXX_STD` CMake flag
- Feature testing: `cMake/ConfigureChecks.cmake`

**Header Configuration:**
- `src/config.h.cmake` - Platform detection and capability defines
- `src/FCConfig.h` - Global feature configuration
- `src/FCGlobal.h` - Exported symbols and version info
- `src/QtCore.h.cmake` - Qt compatibility layer (version abstraction)

**Build Options (ConfigureChecks.cmake):**
- Precompiled headers (PCH): `FREECAD_USE_PCH` (ON by default on supported platforms)
- Compiler sanitizers: AddressSanitizer (ASan), LeakSanitizer (LSan), ThreadSanitizer (TSan), UndefinedBehaviorSanitizer (UBSan), MemorySanitizer (MSan)
- Dynamic linking: `BUILD_DYNAMIC_LINK_PYTHON` - Determines Python library linking strategy
- Ccache integration: `FREECAD_USE_CCACHE` (auto-detected)

**Compiler Flags:**
- Warnings enabled: `-Wall -Wextra -Wno-write-strings` (GCC/Clang)
- Pedantic mode: `-Wpedantic` (Clang only)
- Color diagnostics: `-fdiagnostics-color` (GCC/Clang)
- Undefined symbol resolution: Platform-specific linker flags for dynamic linking safety

## Platform Requirements

**Development:**
- CMake 3.22.0+ (as of 2025-02)
- Git with submodule support
- C++20 compatible compiler (GCC 11.2+, Clang 14.0+, MSVC 2019+)
- Python 3.11+ interpreter and development headers
- Conda/Pixi environment configured with `pixi.toml`

**Build Environment:**
- Linux x86-64 or AArch64 (ARM64)
  - Additional: libx11, libxcb, mesa, libdrm, xcb-util-cursor, xorg-server components (X11 support)
  - Wayland support: qt6-wayland 6.8.x
- macOS 10.13+ (Intel or Apple Silicon)
  - Frameworks: System Preferences, Security setup for development tools
- Windows 10/11
  - MSVC 2019+ with C++20 support
  - pthreads-win32 for threading

**Runtime:**
- Deployment target: macOS 10.13+, Windows 10+, Linux glibc 2.29+
- GUI requires X11/Wayland (Linux) or native compositor (macOS/Windows)
- OpenGL 3.3+ capable graphics hardware
- Approximately 4GB RAM minimum for building, 2GB+ recommended for runtime

---

*Stack analysis: 2026-06-07*
