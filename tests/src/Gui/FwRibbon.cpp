// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <array>
#include <set>
#include <string>
#include <string_view>

#include <Gui/Application.h>
#include <Gui/Command.h>

#include <src/Gui/FreeWorks/FwRibbonMap.h>

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

// Allow-list of curated command IDs that are KNOWN to have no FreeCAD command and
// are accepted as unresolved. It is INTENTIONALLY EMPTY (REVIEW MEDIUM): a
// non-empty allow-list would swallow a typo and defeat the per-row resolution
// guard. If a curated row genuinely has no FreeCAD equivalent, the row is removed
// from FwRibbonMap and recorded there as a `// gap:` comment — never parked here.
constexpr std::array<std::string_view, 0> kKnownGaps = {};

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

// THE typo guard for the pinned curated map (Task 1 done criteria). Iterate EVERY
// row of FwRibbonMap and assert its commandId resolves non-null. An unresolved row
// is a FAILURE (not a silent skip): D-08 silent-omit is RUNTIME robustness, not a
// license for the test to pass on a misspelled/dropped ID. This single test also
// proves the bootstrap imported every owning module — the Evaluate rows
// (Std_Measure/Std_MassProperties via MeasureGui, Part_CheckGeometry via PartGui,
// Materials_Inspect* via MatGui) only resolve if those imports ran
// (REVIEW cycle-2 NEW HIGH 2).
TEST_F(FwRibbonResolutionTest, everyCuratedRowResolves)
{
    std::set<std::string_view> gaps(kKnownGaps.begin(), kKnownGaps.end());
    // The allow-list MUST stay empty — a non-empty one would swallow typos.
    EXPECT_TRUE(gaps.empty()) << "kKnownGaps must be empty (REVIEW MEDIUM): record "
                                 "missing commands as // gap: in FwRibbonMap, not here.";

    bool sawFeatures = false;
    bool sawSketch = false;
    bool sawEvaluate = false;
    bool sawComp = false;

    for (const FreeWorksGui::FwRibbonRow& row : FreeWorksGui::FwRibbonMap::rows()) {
        ASSERT_NE(nullptr, row.tab);
        ASSERT_NE(nullptr, row.panel);
        ASSERT_NE(nullptr, row.commandId);

        const std::string_view tab(row.tab);
        sawFeatures = sawFeatures || (tab == "Features");
        sawSketch = sawSketch || (tab == "Sketch");
        sawEvaluate = sawEvaluate || (tab == "Evaluate");

        const std::string_view id(row.commandId);
        if (id.find("_Comp") != std::string_view::npos) {
            sawComp = true;
        }

        if (gaps.count(id) != 0) {
            continue;  // never reached while kKnownGaps is empty.
        }

        EXPECT_NE(nullptr, resolve(row.commandId))
            << "curated row [" << row.tab << " / " << row.panel << "] command id '"
            << row.commandId << "' did not resolve — typo or a missing owning-module "
                                "import in the GUI test bootstrap.";
    }

    // The three core-loop tabs must all be present in the curated table.
    EXPECT_TRUE(sawFeatures);
    EXPECT_TRUE(sawSketch);
    EXPECT_TRUE(sawEvaluate);
    // At least one flyout (*_Comp*) group id must be present for a downstream split-button.
    EXPECT_TRUE(sawComp);
}
