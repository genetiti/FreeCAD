// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <string>

#include <Gui/Application.h>
#include <Gui/Command.h>

#include "FwTestGuiBootstrap.h"

namespace
{

// Curated command IDs sampled by the resolution tests. These are real IDs owned
// by the modules the bootstrap loads; the ribbon resolves them via
// CommandManager::getCommandByName (Command.h:1011).
constexpr const char* kPartDesignPad = "PartDesign_Pad";
constexpr const char* kPartDesignPocket = "PartDesign_Pocket";
// A *_Comp* group command — the raw material for a native split-button flyout.
constexpr const char* kPartDesignCompPrimitiveSubtractive =
    "PartDesign_CompPrimitiveSubtractive";
// Owned by MeasureGui — proves that the bootstrap's MeasureGui import actually
// loaded (REVIEW cycle-2 NEW HIGH 2), not just PartDesignGui.
constexpr const char* kStdMeasure = "Std_Measure";
// A deliberately non-existent ID — the D-08 omit-missing contract requires
// getCommandByName() to return nullptr (not crash) for it.
constexpr const char* kBogus = "Fw_DoesNotExist";

class FwRibbonResolutionTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        // Creates the Gui::Application singleton THEN loads every owning module's
        // GUI commands, so getCommandByName() below resolves for real instead of
        // returning a false-negative nullptr in a bare FreeCADGui link.
        tests::ensureGuiTestBootstrap();
    }

    static Gui::Command* resolve(const char* id)
    {
        return Gui::Application::Instance->commandManager().getCommandByName(id);
    }
};

}  // namespace

// The GUI singleton must exist after the bootstrap — without it no module command
// would have registered (AppPartDesignGui.cpp:103-105).
TEST_F(FwRibbonResolutionTest, guiSingletonExistsAfterBootstrap)
{
    EXPECT_NE(nullptr, Gui::Application::Instance);
}

// A curated PartDesign command resolves to a non-null Gui::Command — proves the
// PartDesignGui module GUI load ran CreatePartDesignCommands().
TEST_F(FwRibbonResolutionTest, curatedPartDesignIdResolves)
{
    EXPECT_NE(nullptr, resolve(kPartDesignPad));
    EXPECT_NE(nullptr, resolve(kPartDesignPocket));
}

// A command owned by MeasureGui resolves — proves the bootstrap's MeasureGui
// import loaded, not merely PartDesignGui (REVIEW cycle-2 NEW HIGH 2).
TEST_F(FwRibbonResolutionTest, measureGuiCommandResolves)
{
    EXPECT_NE(nullptr, resolve(kStdMeasure));
}

// A *_Comp* group command resolves — the raw material for a native split-button.
TEST_F(FwRibbonResolutionTest, groupCommandResolves)
{
    EXPECT_NE(nullptr, resolve(kPartDesignCompPrimitiveSubtractive));
}

// D-08 omit-missing contract: a bogus ID resolves to nullptr, no crash.
TEST_F(FwRibbonResolutionTest, bogusIdResolvesToNull)
{
    EXPECT_EQ(nullptr, resolve(kBogus));
}
