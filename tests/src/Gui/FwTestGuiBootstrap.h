// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <array>
#include <cassert>

#include <QApplication>

#include <App/Application.h>
#include <Base/Interpreter.h>
#include <Gui/Application.h>

#include <src/App/InitApplication.h>

namespace tests
{

// Shared GUI test bootstrap for the FreeWorks ribbon tests.
//
// ensureGuiTestBootstrap() is idempotent and performs four steps IN THIS ORDER:
//
//   (1) initialise App::Application (tests::initApplication()).
//   (2) ensure a process-lifetime QApplication exists. The setup_qt_test harness
//       sets QT_QPA_PLATFORM=offscreen for the Qt (QTEST_MAIN) targets; for any
//       caller without that env we set the offscreen platform before constructing
//       so the bootstrap never needs a display. The QApplication is a function
//       static (with static argc/argv) so it lives for the whole process and is
//       NEVER a local temporary destroyed mid-test.
//   (3) CREATE the Gui::Application singleton BEFORE importing any GUI module:
//       `if (!Gui::Application::Instance) new Gui::Application(false);`. This
//       mirrors FreeCADGui.setupWithoutGUI() (FreeCADGuiPy.cpp:200-201); the
//       `false` (GUI-disabled) constructor sets `Instance = this`
//       (Application.cpp:746). This step is LOAD-BEARING: skipping it makes the
//       module imports in step 4 raise
//       ImportError("Cannot load Gui module in console application.") from the
//       `if (!Gui::Application::Instance)` guard at AppPartDesignGui.cpp:103-105,
//       and zero commands register.
//   (4) import EVERY GUI module that OWNS a curated command ID:
//         - PartDesignGui — pulls PartGui + SketcherGui internally
//           (AppPartDesignGui.cpp:111-112), covering Part_CheckGeometry, and runs
//           CreatePartDesignCommands() (AppPartDesignGui.cpp:122) which registers
//           the PartDesign_* IDs.
//         - SketcherGui    — Sketcher_* IDs.
//         - MeasureGui     — Std_Measure / Std_MassProperties
//           (Measure/Gui/Command.cpp:48,94).
//         - MatGui         — Materials_Inspect* (Material/Gui/Command.cpp).
//       Each import is wrapped so one module gated out by a BUILD_* option does
//       not abort the others (PartDesign/Sketcher/Measure are baseline).
//
// After it runs once, Gui::Application::Instance != nullptr,
// getCommandByName("PartDesign_Pad") returns non-null, and
// getCommandByName("Std_Measure") returns non-null.
inline void ensureGuiTestBootstrap()
{
    static bool done = false;
    if (done) {
        return;
    }

    // (1) App::Application.
    tests::initApplication();

    // (2) QApplication (offscreen). Kept alive for the whole process.
    if (QApplication::instance() == nullptr) {
        static int argc = 1;
        static std::array<char, 8> arg0 {'F', 'r', 'e', 'e', 'C', 'A', 'D', '\0'};
        static std::array<char*, 1> argv {arg0.data()};
        // The setup_qt_test harness exports QT_QPA_PLATFORM=offscreen; set it here
        // too so a non-Qt caller still runs without a display.
        qputenv("QT_QPA_PLATFORM", "offscreen");
        static QApplication app(argc, argv.data());
        (void)app;
    }

    // (3) Gui::Application singleton — MUST precede the module imports below.
    if (!Gui::Application::Instance) {
        static Gui::Application* guiApp = new Gui::Application(false);
        (void)guiApp;
    }

    // Fail loudly if the singleton did not come up: a regressed bootstrap must
    // not silently let the resolution asserts pass against a null manager.
    assert(Gui::Application::Instance != nullptr
           && "Gui::Application singleton must exist before GUI-module import");

    // (4) Import every GUI module that owns a curated command ID. Tolerate a
    //     module that is gated out by its BUILD_* option (optional ones only).
    auto tryImport = [](const char* module) {
        try {
            Base::Interpreter().runString((std::string("import ") + module).c_str());
        }
        catch (const Base::Exception&) {
            // An optional module may be absent in a trimmed build; baseline
            // modules (PartDesignGui/SketcherGui/MeasureGui) are expected present.
        }
    };

    tryImport("PartDesignGui");  // pulls PartGui + SketcherGui internally
    tryImport("SketcherGui");
    tryImport("MeasureGui");  // owns Std_Measure / Std_MassProperties
    tryImport("MatGui");  // owns Materials_Inspect*

    done = true;
}

}  // namespace tests
