// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <memory>
#include <set>
#include <string>

#include <src/App/InitApplication.h>

#include <App/Application.h>
#include <Base/Parameter.h>
#include <Gui/DockWindowManager.h>
#include <Gui/Workbench.h>

#include <src/Gui/FreeWorks/FwWorkbench.h>
#include <src/Gui/FreeWorks/FwNavigationDefault.h>

namespace
{

// Permanent FreeWorks dock names registered by FwWorkbench::setupDockWindows().
// Later phases swap the content behind these names, never the names themselves.
constexpr const char* kFwFeatureManager = "Fw_FeatureManager";
constexpr const char* kFwPropertyManager = "Fw_PropertyManager";
constexpr const char* kFwTaskPane = "Fw_TaskPane";

// The single upstream navigation-style type-name string (D-03 exception). When
// the NavigationStyle preference is unset, FwNavigationDefault defaults to this.
constexpr const char* kNavStyleKey = "NavigationStyle";
constexpr const char* kSolidWorksNavStyle = "Gui::SolidWorksNavigationStyle";
constexpr const char* kViewPrefGroup = "User parameter:BaseApp/Preferences/View";

// setupDockWindows() is protected on Gui::Workbench (and on FwWorkbench). Rather
// than widen production visibility, expose it for the test via a subclass that
// re-publishes the protected member with a using-declaration — the project's
// documented convention for testing protected members.
class FwWorkbenchAccessor: public FreeWorksGui::FwWorkbench
{
public:
    using FreeWorksGui::FwWorkbench::setupDockWindows;  // expose for test
};

class FwWorkbenchTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        // Snapshot the NavigationStyle key so the no-clobber test can restore it
        // and leave shared preference state untouched for sibling gtests.
        _viewGrp = App::GetApplication().GetParameterGroupByPath(kViewPrefGroup);
        _savedNavStyle = _viewGrp->GetASCII(kNavStyleKey, "");
        _savedNavStylePresent = !_savedNavStyle.empty();
    }

    void TearDown() override
    {
        if (_savedNavStylePresent) {
            _viewGrp->SetASCII(kNavStyleKey, _savedNavStyle.c_str());
        }
        else {
            _viewGrp->RemoveASCII(kNavStyleKey);
        }
    }

    ParameterGrp::handle _viewGrp;
    std::string _savedNavStyle;
    bool _savedNavStylePresent {false};
};

}  // namespace

// SC1: FwWorkbench is type-registered and instantiable, and setupDockWindows()
// returns the three permanent Fw_* dock names.
TEST_F(FwWorkbenchTest, registersAndExposesFwDockNames)
{
    // Type registration: the workbench is known to FreeCAD's type system.
    Base::Type type = FreeWorksGui::FwWorkbench::getClassTypeId();
    ASSERT_FALSE(type.isBad());
    EXPECT_EQ(std::string("FreeWorksGui::FwWorkbench"), std::string(type.getName()));
    EXPECT_TRUE(type.isDerivedFrom(Gui::StdWorkbench::getClassTypeId()));

    // Instantiable directly (headless: no view, no MainWindow interaction).
    // Use the accessor subclass to reach the protected setupDockWindows().
    FwWorkbenchAccessor workbench;

    // setupDockWindows() registers the permanent Fw_* dock names.
    std::unique_ptr<Gui::DockWindowItems> docks(workbench.setupDockWindows());
    ASSERT_NE(nullptr, docks);

    std::set<std::string> names;
    for (const Gui::DockWindowItem& item : docks->dockWidgets()) {
        names.insert(item.name.toStdString());
    }

    EXPECT_TRUE(names.count(kFwFeatureManager) == 1)
        << "missing permanent dock name " << kFwFeatureManager;
    EXPECT_TRUE(names.count(kFwPropertyManager) == 1)
        << "missing permanent dock name " << kFwPropertyManager;
    EXPECT_TRUE(names.count(kFwTaskPane) == 1)
        << "missing permanent dock name " << kFwTaskPane;
}

// SC3: with the NavigationStyle key unset, applyDefault() defaults it to the
// upstream reference-CAD navigation style.
TEST_F(FwWorkbenchTest, navigationStyleDefaultsToSolidWorksWhenUnset)
{
    _viewGrp->RemoveASCII(kNavStyleKey);
    ASSERT_TRUE(_viewGrp->GetASCII(kNavStyleKey, "").empty());

    FreeWorksGui::FwNavigationDefault::applyDefault();

    EXPECT_EQ(std::string(kSolidWorksNavStyle), _viewGrp->GetASCII(kNavStyleKey, ""));
}

// Edge (note A2): a pre-set, user-chosen NavigationStyle is never overwritten.
TEST_F(FwWorkbenchTest, navigationStyleDoesNotClobberUserChoice)
{
    const std::string userChoice = "Gui::CADNavigationStyle";
    _viewGrp->SetASCII(kNavStyleKey, userChoice.c_str());

    FreeWorksGui::FwNavigationDefault::applyDefault();

    EXPECT_EQ(userChoice, _viewGrp->GetASCII(kNavStyleKey, ""))
        << "applyDefault() must not clobber a user-chosen navigation style";
}
