// SPDX-License-Identifier: LGPL-2.1-or-later

// Wave 0 offscreen QTEST scaffold for the Phase-4 PropertyManager (Plan 04-01).
//
// Constructing/holding a real Gui::MainWindow and resolving the registered Tasks
// TaskView dock REQUIRES a live QApplication, so this construction harness lives in a
// QTEST_MAIN Qt target registered via setup_qt_test() (which sets
// QT_QPA_PLATFORM=offscreen and creates the FwPropertyManagerWidget_Tests_run
// executable) — it cannot run in the bare Gui_tests_run GTest target. This mirrors
// the Phase-2 FwRibbonWidget / Phase-3 FwFeatureTreeWidget split exactly.
//
// At this Wave 0 stage there is NO FreeWorks production mount yet
// (FwLayout::mountPropertyManager() lands in Plan 04-02). In FreeWorks mode
// Control().taskPanel() is null until that mount runs (F1: FwWorkbench::setupDockWindows()
// never returns Std_TaskView, so the framework builds no live "Tasks" dock). So this
// scaffold asserts the BASELINE the Plan 04-02 left-placement test extends: a real
// MainWindow can bring up and REGISTER the Tasks TaskView (objectName "Tasks", registry
// key "Std_TaskView" — MainWindow.cpp:620-628), and that registered widget is
// resolvable offscreen. Plan 04-02 then asserts Control().taskPanel() lands in the LEFT
// area for BOTH the never-docked and came-from-right-dock cases.

#include <QApplication>
#include <QTest>
#include <QWidget>

#include <Gui/Application.h>
#include <Gui/DockWindowManager.h>
#include <Gui/MainWindow.h>
#include <Gui/TaskView/TaskView.h>

#include "FwTestGuiBootstrap.h"

namespace
{
// Ensure a real Gui::MainWindow exists and is registered as the FreeCAD singleton.
// Constructing Gui::MainWindow sets MainWindow::instance = this (MainWindow.cpp:358),
// so Gui::getMainWindow() returns it afterwards — a plain QMainWindow does NOT register
// the singleton (mirror FwRibbonWidget.cpp:37-43). The window is leaked deliberately: it
// must outlive the whole QTEST_MAIN process so later test functions and Qt teardown
// still see a valid singleton.
Gui::MainWindow* ensureRealMainWindow()
{
    if (Gui::getMainWindow() == nullptr) {
        new Gui::MainWindow();  // sets MainWindow::instance = this
    }
    return Gui::getMainWindow();
}
}  // namespace

class testFwPropertyManagerWidget: public QObject
{
    Q_OBJECT

public:
    testFwPropertyManagerWidget()
    {
        // Brings up App::Application, the offscreen QApplication, the Gui::Application
        // singleton, and loads the owning GUI modules so Control / TaskView / Selection
        // and the SketcherGui::ViewProviderSketch type resolve.
        tests::ensureGuiTestBootstrap();
    }

private Q_SLOTS:

    void initTestCase()
    {
        // A real Gui::MainWindow must be the FreeCAD singleton before any dock lookup —
        // the registered Tasks dock is owned by the MainWindow (MainWindow.cpp:609-633).
        Gui::MainWindow* mw = ensureRealMainWindow();
        QVERIFY2(mw != nullptr, "a real Gui::MainWindow must be the FreeCAD singleton");
        QCOMPARE(static_cast<void*>(mw), static_cast<void*>(Gui::getMainWindow()));
    }

    void init()
    {}

    void cleanup()
    {}

    // The QApplication the QTEST_MAIN harness provides must be live (offscreen) — every
    // QWidget/QDockWidget construction below depends on it.
    void test_qApplicationIsLive()
    {
        QVERIFY2(QApplication::instance() != nullptr,
                 "the offscreen QApplication must be live for QWidget construction");
    }

    // The BASELINE the Plan 04-02 left-placement test extends: a real MainWindow can
    // register the Tasks TaskView, and that registered widget is resolvable offscreen
    // via the DockWindowManager. setupTaskView() creates a Gui::TaskView::TaskView with
    // objectName "Tasks" and registers it under the key "Std_TaskView"
    // (MainWindow.cpp:620-628). (F1: in FreeWorks mode Control().taskPanel() is null
    // until Plan 04-02's mount runs — so this scaffold asserts resolvability of the
    // registered widget, NOT a live left dock.)
    void test_bootstrap_taskView_resolvable()
    {
        Gui::MainWindow* mw = ensureRealMainWindow();
        QVERIFY(mw != nullptr);

        auto* mgr = Gui::DockWindowManager::instance();
        QVERIFY2(mgr != nullptr, "the DockWindowManager singleton must exist");

        // Idempotent: setupTaskView() is a no-op if Std_TaskView was already set up /
        // hidden. Run it so the Tasks TaskView is registered for the lookup below.
        mw->setupTaskView();

        QWidget* registered = mgr->findRegisteredDockWindow("Std_TaskView");
        QVERIFY2(registered != nullptr,
                 "the Tasks TaskView must be resolvable via the Std_TaskView registry key");

        // The registered widget is a Gui::TaskView::TaskView whose objectName is "Tasks"
        // (the identity Control::taskPanel()'s getDockWindow("Tasks") depends on —
        // Plan 04-02's left placement must preserve it).
        auto* taskView = qobject_cast<Gui::TaskView::TaskView*>(registered);
        QVERIFY2(taskView != nullptr, "the registered Std_TaskView widget must be a TaskView");
        QCOMPARE(taskView->objectName(), QStringLiteral("Tasks"));
    }
};

QTEST_MAIN(testFwPropertyManagerWidget)

#include "FwPropertyManagerWidget.moc"
