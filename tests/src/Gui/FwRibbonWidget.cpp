// SPDX-License-Identifier: LGPL-2.1-or-later

#include <list>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <QList>
#include <QMenu>
#include <QString>
#include <QTest>
#include <QToolBar>
#include <QToolButton>
#include <QWidget>

#include <Gui/Application.h>
#include <Gui/Command.h>

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

    // RIBBON-01 SC1: the full curated map builds exactly the three core-loop tabs
    // (Features / Sketch / Evaluate), each hosting at least one panel.
    void test_CuratedBuildYieldsThreeTabs()
    {
        auto ribbon = std::make_unique<FreeWorksGui::FwRibbon>();
        ribbon->buildFromCuratedMap();

        QCOMPARE(ribbon->tabCount(), 3);

        std::set<QString> tabs;
        for (int i = 0; i < ribbon->count(); ++i) {
            tabs.insert(ribbon->tabText(i));
            // Each curated tab page must carry at least one panel QToolBar.
            QVERIFY(!ribbon->widget(i)->findChildren<QToolBar*>().isEmpty());
        }
        QVERIFY(tabs.count(QStringLiteral("Features")) == 1);
        QVERIFY(tabs.count(QStringLiteral("Sketch")) == 1);
        QVERIFY(tabs.count(QStringLiteral("Evaluate")) == 1);
    }

    // RIBBON-01 SC3 + REVIEW concern 8: a *_Comp* group command, when added to a
    // QToolBar via the SAME addTo() path the ribbon uses, produces a REAL
    // QToolButton with popupMode()==MenuButtonPopup and a menu of >1 actions. This
    // inspects the actual widget output, NOT getGroupCommands() metadata.
    void test_FlyoutProducesMenuButtonPopup()
    {
        Gui::Command* cmd = Gui::Application::Instance->commandManager().getCommandByName(
            "PartDesign_CompPrimitiveSubtractive");
        QVERIFY2(cmd != nullptr, "the *_Comp* group command must resolve after bootstrap");

        auto toolbar = std::make_unique<QToolBar>();
        toolbar->setObjectName(QStringLiteral("Fw_FlyoutProbe"));
        cmd->addTo(toolbar.get());

        // Find the split-button the group command produced on the toolbar.
        const QList<QToolButton*> buttons = toolbar->findChildren<QToolButton*>();
        QToolButton* split = nullptr;
        for (QToolButton* btn : buttons) {
            if (btn->menu() != nullptr
                && btn->popupMode() == QToolButton::MenuButtonPopup) {
                split = btn;
                break;
            }
        }
        QVERIFY2(split != nullptr, "expected a MenuButtonPopup split-button from the group id");
        QCOMPARE(split->popupMode(), QToolButton::MenuButtonPopup);
        QVERIFY2(split->menu()->actions().size() > 1,
                 "the flyout menu must offer more than one action");
    }

    // D-07: auto-derive from a synthesized getToolbarItems()-shaped list. Two groups
    // become two panels; a "Separator" sentinel inside a group inserts a separator
    // (not a button) — REVIEW concern 5. All panels live under the "Tools" tab.
    void test_AutoDerivePanelAndSeparatorCount()
    {
        std::list<std::pair<std::string, std::list<std::string>>> groups {
            {"Primitives",
             {"PartDesign_Pad", "Separator", "PartDesign_Pocket"}},
            {"Reference", {"PartDesign_Body"}},
        };

        auto ribbon = std::make_unique<FreeWorksGui::FwRibbon>();
        ribbon->buildAutoDerived(groups);

        QCOMPARE(ribbon->tabCount(), 1);
        QWidget* page = ribbon->widget(0);
        QCOMPARE(ribbon->tabText(0), QStringLiteral("Tools"));

        const QList<QToolBar*> panels = page->findChildren<QToolBar*>();
        QCOMPARE(panels.size(), 2);

        // The "Primitives" panel must contain exactly one separator action.
        QToolBar* primitives = nullptr;
        for (QToolBar* bar : panels) {
            if (bar->objectName() == QStringLiteral("Fw_RibbonPanel_Tools_Primitives")) {
                primitives = bar;
                break;
            }
        }
        QVERIFY(primitives != nullptr);
        int separators = 0;
        for (QAction* act : primitives->actions()) {
            if (act->isSeparator()) {
                ++separators;
            }
        }
        QCOMPARE(separators, 1);
    }

    // Pitfall 4 / D-14 prep: every persisted panel QToolBar carries a unique
    // Fw_RibbonPanel_<Tab>_<Panel> objectName so saveState()/restoreState() can key.
    void test_PanelObjectNamesAreUnique()
    {
        auto ribbon = std::make_unique<FreeWorksGui::FwRibbon>();
        ribbon->buildFromCuratedMap();

        std::set<QString> names;
        for (int i = 0; i < ribbon->count(); ++i) {
            for (QToolBar* bar : ribbon->widget(i)->findChildren<QToolBar*>()) {
                const QString name = bar->objectName();
                QVERIFY2(name.startsWith(QStringLiteral("Fw_RibbonPanel_")),
                         "panel objectName must use the Fw_RibbonPanel_ prefix");
                QVERIFY2(names.insert(name).second,
                         "panel objectNames must be unique across the ribbon");
            }
        }
        QVERIFY(!names.empty());
    }

    // Plan 04 seam: setCurrentTab(name) selects the matching tab; an absent name is
    // a tolerated no-op.
    void test_SetCurrentTabSelectsByName()
    {
        auto ribbon = std::make_unique<FreeWorksGui::FwRibbon>();
        ribbon->buildFromCuratedMap();

        ribbon->setCurrentTab(QStringLiteral("Sketch"));
        QCOMPARE(ribbon->tabText(ribbon->currentIndex()), QStringLiteral("Sketch"));

        const int before = ribbon->currentIndex();
        ribbon->setCurrentTab(QStringLiteral("NoSuchTab"));
        QCOMPARE(ribbon->currentIndex(), before);  // unchanged.
    }
};

QTEST_MAIN(testFwRibbonWidget)

#include "FwRibbonWidget.moc"
