// SPDX-License-Identifier: LGPL-2.1-or-later

#include <memory>
#include <string>
#include <vector>

#include <QTest>

#include <src/Gui/FreeWorks/FwRibbon.h>

#include "FwTestGuiBootstrap.h"

// FwRibbon is a QTabWidget; constructing it REQUIRES a live QApplication. The
// plain Gui_tests_run GTest target has none, so the widget-construction assertion
// lives here in a QTEST_MAIN Qt target (registered via setup_qt_test, which sets
// QT_QPA_PLATFORM=offscreen) — see REVIEW concern 1.
class testFwRibbonWidget: public QObject
{
    Q_OBJECT

public:
    testFwRibbonWidget()
    {
        // Brings up App::Application, the QApplication, the Gui::Application
        // singleton, and loads the owning GUI modules so the curated IDs resolve.
        tests::ensureGuiTestBootstrap();
    }

private Q_SLOTS:

    void init()
    {}

    void cleanup()
    {}

    // RIBBON-01 build assertion: an FwRibbon can be constructed headless and one
    // call to addTabFromCommandIds() with two resolvable IDs yields exactly one
    // tab. Requires the live QApplication that QTEST_MAIN provides.
    void test_BuildsSingleTabFromCommandIds()
    {
        auto ribbon = std::make_unique<FreeWorksGui::FwRibbon>();
        QCOMPARE(ribbon->tabCount(), 0);

        const std::vector<std::string> ids {"PartDesign_Pad", "PartDesign_Pocket"};
        ribbon->addTabFromCommandIds(QStringLiteral("Features"), ids);

        QCOMPARE(ribbon->tabCount(), 1);
    }

    // D-08 omit-missing: a tab built entirely from unresolved IDs still produces a
    // tab page (no crash); unresolved IDs are silently skipped.
    void test_SkipsUnresolvedIds()
    {
        auto ribbon = std::make_unique<FreeWorksGui::FwRibbon>();

        const std::vector<std::string> ids {"Fw_DoesNotExist", "PartDesign_Pad"};
        ribbon->addTabFromCommandIds(QStringLiteral("Mixed"), ids);

        QCOMPARE(ribbon->tabCount(), 1);
    }
};

QTEST_MAIN(testFwRibbonWidget)

#include "FwRibbonWidget.moc"
