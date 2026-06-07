# SPDX-License-Identifier: LGPL-2.1-or-later

# FreeWorks: additive reference-CAD-style Gui mode for FreeCAD.
#
# This module is intentionally additive. Its sources compile directly into the
# existing FreeCADGui shared library (the target is created earlier in
# src/Gui/CMakeLists.txt), so no new link target and no MainWindow edits are
# required. The ONLY shared-file touch is the single marked add_subdirectory
# line in src/Gui/CMakeLists.txt.

# Resources: workbench icon bundle (recreated-original art; provenance in Plan 04).
set(FreeWorks_RES_SRCS
    Resources/FreeWorks.qrc
)
qt_add_resources(FreeWorks_QRC_SRCS ${FreeWorks_RES_SRCS})

set(FreeWorks_CPP_SRCS
    PreCompiled.cpp
    FwTheme.cpp
    FwLayout.cpp
    FwNavigationDefault.cpp
    FwWorkbench.cpp
    FwRibbon.cpp
)

set(FreeWorks_HPP_SRCS
    PreCompiled.h
    FwTheme.h
    FwLayout.h
    FwNavigationDefault.h
    FwWorkbench.h
    FwRibbon.h
)

set(FreeWorks_SRCS
    ${FreeWorks_CPP_SRCS}
    ${FreeWorks_HPP_SRCS}
    ${FreeWorks_QRC_SRCS}
)

source_group("FreeWorks" FILES ${FreeWorks_SRCS})

# Compile the FreeWorks sources straight into FreeCADGui (additive, no new lib).
target_sources(FreeCADGui PRIVATE ${FreeWorks_SRCS})

# InitGui.py registers the FreeWorks workbench through FreeCAD's standard
# Mod/<name>/InitGui.py discovery path. Copy it into the build tree and install it.
set(FreeWorks_Scripts
    InitGui.py
)

add_custom_target(FreeWorksScripts ALL SOURCES ${FreeWorks_Scripts})

fc_target_copy_resource(FreeWorksScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/FreeWorks
    ${FreeWorks_Scripts}
)

install(
    FILES ${FreeWorks_Scripts}
    DESTINATION Mod/FreeWorks
)
