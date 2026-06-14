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
#include <Gui/MainWindow.h>

#include <src/Gui/FreeWorks/FwLayout.h>
#include <src/Gui/FreeWorks/FwRibbon.h>

#include "FwTestGuiBootstrap.h"

namespace
{
// Ensure a real Gui::MainWindow exists and is registered as the FreeCAD singleton.
// Constructing Gui::MainWindow sets MainWindow::instance = this (MainWindow.cpp:358),
// so Gui::getMainWindow() returns it afterwards — a plain QMainWindow does NOT
// register the singleton, so a test built against one can pass while the real
// Gui::getMainWindow()-based mount is broken (REVIEW cycle-2 NEW HIGH 3). The
// window is leaked deliberately: it must outlive the whole QTEST_MAIN process so
// later test functions and Qt teardown still see a valid singleton.
Gui::MainWindow* ensureRealMainWindow()
{
    if (Gui::getMainWindow() == nullptr) {
        new Gui::MainWindow();  // sets MainWindow::instance = this
    }
    return Gui::getMainWindow();
}
}  // namespace

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

    // RIBBON-01 SC1 (Task 1): mountRibbon() wraps the ribbon in a REAL QToolBar
    // (objectName Fw_RibbonToolBar) and adds it to the FreeCAD main window's
    // Qt::TopToolBarArea — the REVIEW-corrected seam (concerns 3 & 4). This
    // exercises the PRODUCTION path: mountRibbon() calls Gui::getMainWindow()
    // internally, and we assert against that SAME real Gui::MainWindow (NOT a bare
    // QMainWindow — REVIEW cycle-2 NEW HIGH 3).
    void test_MountAddsRealToolBarInTopArea()
    {
        Gui::MainWindow* mw = ensureRealMainWindow();
        QVERIFY2(mw != nullptr, "a real Gui::MainWindow must be the FreeCAD singleton");
        QCOMPARE(static_cast<void*>(mw), static_cast<void*>(Gui::getMainWindow()));

        FreeWorksGui::FwLayout::mountRibbon();

        QToolBar* tb = mw->findChild<QToolBar*>(QStringLiteral("Fw_RibbonToolBar"));
        QVERIFY2(tb != nullptr, "mountRibbon() must add a Fw_RibbonToolBar wrapper");
        // The wrapper must host the Fw_Ribbon QTabWidget.
        QVERIFY2(tb->findChild<FreeWorksGui::FwRibbon*>() != nullptr
                     || mw->findChild<QWidget*>(QStringLiteral("Fw_Ribbon")) != nullptr,
                 "the wrapper must host the Fw_Ribbon widget");
        // Assert the ribbon is a genuine TopToolBarArea participant, not merely a
        // child somewhere — this is the saveState() round-trip prerequisite.
        QCOMPARE(mw->toolBarArea(tb), Qt::TopToolBarArea);

        FreeWorksGui::FwLayout::unmountRibbon();
    }

    // REVIEW MEDIUM: mountRibbon() is idempotent — re-activating FreeWorks must not
    // create a second Fw_RibbonToolBar wrapper.
    void test_MountIsIdempotent()
    {
        Gui::MainWindow* mw = ensureRealMainWindow();
        QVERIFY(mw != nullptr);

        FreeWorksGui::FwLayout::mountRibbon();
        FreeWorksGui::FwLayout::mountRibbon();

        const QList<QToolBar*> wrappers =
            mw->findChildren<QToolBar*>(QStringLiteral("Fw_RibbonToolBar"));
        QCOMPARE(wrappers.size(), 1);

        FreeWorksGui::FwLayout::unmountRibbon();
    }

    // D-11: hideStockChrome()/restoreStockChrome() round-trip the reachable chrome
    // state headless. The live menu-bar + macOS native-menu round-trip stays a
    // VALIDATION manual item, but the menu-bar visibility toggle is assertable here.
    void test_ChromeHideRestoreRoundTrip()
    {
        Gui::MainWindow* mw = ensureRealMainWindow();
        QVERIFY(mw != nullptr);
        QMenuBar* mb = mw->menuBar();
        QVERIFY(mb != nullptr);

        mb->setVisible(true);
        const bool before = mb->isVisible();

        FreeWorksGui::FwLayout::hideStockChrome();
        QVERIFY2(!mb->isVisible(), "hideStockChrome() must hide the stock menu bar");

        FreeWorksGui::FwLayout::restoreStockChrome();
        QCOMPARE(mb->isVisible(), before);
    }

    // D-12 (Task 2): the ribbon installs a pinned "More commands…" overflow as the
    // top-right tab-bar corner widget (a QToolButton with a menu), reaching the
    // uncurated command set. Asserts the affordance EXISTS and is labeled exactly.
    void test_OverflowCornerWidgetPresent()
    {
        auto ribbon = std::make_unique<FreeWorksGui::FwRibbon>();
        ribbon->buildFromCuratedMap();

        QWidget* corner = ribbon->cornerWidget(Qt::TopRightCorner);
        QVERIFY2(corner != nullptr, "the overflow corner widget must be installed");
        auto* button = qobject_cast<QToolButton*>(corner);
        QVERIFY2(button != nullptr, "the overflow affordance must be a QToolButton");
        QCOMPARE(button->text(), QStringLiteral("More commands…"));
        QVERIFY2(button->menu() != nullptr, "the overflow button must carry a menu");

        // Lazily building the menu reaches the uncurated command set (>0 entries).
        button->menu()->aboutToShow();  // trigger the lazy rebuild
        QVERIFY2(!button->menu()->actions().isEmpty(),
                 "the overflow menu must reach the uncurated command set");
    }

    // D-14 (Task 2) toolbar layer: the wrapper QToolBar round-trips through the real
    // Gui::MainWindow's saveState()/restoreState() — assert the RESTORED AREA
    // (Qt::TopToolBarArea), not mere objectName existence (REVIEW MEDIUM). Uses the
    // production seam (Gui::getMainWindow()), not a bare generic QMainWindow.
    void test_ToolBarStateRoundTripRestoresArea()
    {
        Gui::MainWindow* mw = ensureRealMainWindow();
        QVERIFY(mw != nullptr);

        FreeWorksGui::FwLayout::mountRibbon();
        QToolBar* tb = mw->findChild<QToolBar*>(QStringLiteral("Fw_RibbonToolBar"));
        QVERIFY2(tb != nullptr, "the wrapper toolbar must be mounted before saveState");
        QCOMPARE(mw->toolBarArea(tb), Qt::TopToolBarArea);

        const QByteArray blob = mw->saveState();
        QVERIFY2(!blob.isEmpty(), "saveState() must serialize the real QToolBar wrapper");

        // Round-trip: restoreState must place the wrapper back in the top area.
        QVERIFY2(mw->restoreState(blob), "restoreState() must accept the saved blob");
        QToolBar* restored = mw->findChild<QToolBar*>(QStringLiteral("Fw_RibbonToolBar"));
        QVERIFY(restored != nullptr);
        QCOMPARE(mw->toolBarArea(restored), Qt::TopToolBarArea);

        FreeWorksGui::FwLayout::unmountRibbon();
    }

    // D-14 (Task 2) tab layer: the SELECTED TAB persists separately via the
    // ParameterGrp key (QMainWindow::saveState does NOT cover it — REVIEW concern 4).
    // Asserts the RESTORED INDEX, not object existence.
    void test_TabStateRoundTripRestoresIndex()
    {
        auto ribbon = std::make_unique<FreeWorksGui::FwRibbon>();
        ribbon->buildFromCuratedMap();
        QVERIFY2(ribbon->tabCount() >= 3, "curated build must yield the 3 core tabs");

        ribbon->setCurrentIndex(2);
        ribbon->saveTabState();

        // A fresh ribbon must pick the persisted index back up after a build.
        auto fresh = std::make_unique<FreeWorksGui::FwRibbon>();
        fresh->buildFromCuratedMap();  // build() calls restoreTabState() internally
        QCOMPARE(fresh->currentIndex(), 2);

        // Explicit restoreTabState() is also idempotent.
        fresh->setCurrentIndex(0);
        fresh->restoreTabState();
        QCOMPARE(fresh->currentIndex(), 2);
    }
};

QTEST_MAIN(testFwRibbonWidget)

#include "FwRibbonWidget.moc"
