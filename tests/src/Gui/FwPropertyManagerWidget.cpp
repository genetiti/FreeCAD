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
#include <QDockWidget>
#include <QTest>
#include <QWidget>

#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/DockWindowManager.h>
#include <Gui/MainWindow.h>
#include <Gui/TaskView/TaskView.h>

#include <src/Gui/FreeWorks/FwLayout.h>

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

// Walk up from the registered/hosted Tasks TaskView to its parent QDockWidget. The
// managed PropertyManager dock identity is the CONTAINER named "Tasks" (R2-F3); its
// area is what mountPropertyManager() must drive to Qt::LeftDockWidgetArea.
QDockWidget* tasksParentDock()
{
    Gui::TaskView::TaskView* tv = Gui::Control().taskPanel();
    return tv != nullptr ? qobject_cast<QDockWidget*>(tv->parentWidget()) : nullptr;
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

    // --- Plan 04-02: FwLayout::mountPropertyManager() left placement -----------

    // Never-docked case (FreeWorks mode, F1): with no live "Tasks" dock yet,
    // mountPropertyManager() resolves the registered Std_TaskView widget and CREATES
    // the dock left via addDockWindow("Tasks", taskView, Left). After the mount
    // Control().taskPanel() is non-null and its parent QDockWidget is in the LEFT area
    // (R2-F1 create-left branch), and NO Fw_PropertyManager dock remains (R3-MAJOR3).
    void test_mount_neverDocked_createsLeft()
    {
        Gui::MainWindow* mw = ensureRealMainWindow();
        QVERIFY(mw != nullptr);
        mw->setupTaskView();  // register the Tasks TaskView for the lookup

        FreeWorksGui::FwLayout::mountPropertyManager();

        QVERIFY2(Gui::Control().taskPanel() != nullptr,
                 "after mount Control().taskPanel() must resolve the hosted Tasks panel");
        QDockWidget* dock = tasksParentDock();
        QVERIFY2(dock != nullptr, "the hosted Tasks panel must live in a QDockWidget after mount");
        QCOMPARE(mw->dockWidgetArea(dock), Qt::LeftDockWidgetArea);
        QCOMPARE(dock->objectName(), QStringLiteral("Tasks"));

        // R3-MAJOR3: no stale/second left surface from the placeholder.
        QVERIFY2(Gui::DockWindowManager::instance()->getDockWindow("Fw_PropertyManager") == nullptr,
                 "no Fw_PropertyManager dock may remain after mount (single left surface)");
    }

    // Came-from-right-dock case (R2-F1): pre-dock the Tasks widget on the RIGHT (as the
    // stock Std_TaskView does, Workbench.cpp:925), then mountPropertyManager() must
    // RE-DOCK the EXISTING container LEFT via getMainWindow()->addDockWidget(Left, dock)
    // — addDockWindow alone CANNOT move an already-docked panel
    // (DockWindowManager.cpp:256-258).
    void test_mount_cameFromRightDock_reDocksLeft()
    {
        Gui::MainWindow* mw = ensureRealMainWindow();
        QVERIFY(mw != nullptr);
        mw->setupTaskView();

        // Ensure a live "Tasks" container exists docked RIGHT (simulate Std_TaskView).
        FreeWorksGui::FwLayout::mountPropertyManager();
        QDockWidget* dock = tasksParentDock();
        QVERIFY(dock != nullptr);
        mw->addDockWidget(Qt::RightDockWidgetArea, dock);
        QCOMPARE(mw->dockWidgetArea(dock), Qt::RightDockWidgetArea);

        // Re-mount: must re-dock the EXISTING container left, preserving identity.
        FreeWorksGui::FwLayout::mountPropertyManager();
        QDockWidget* afterDock = tasksParentDock();
        QVERIFY(afterDock != nullptr);
        QCOMPARE(mw->dockWidgetArea(afterDock), Qt::LeftDockWidgetArea);
        QCOMPARE(afterDock->objectName(), QStringLiteral("Tasks"));
        QVERIFY2(Gui::Control().taskPanel() != nullptr,
                 "taskPanel() must keep resolving after the re-dock (parent-independent)");
    }

    // saveState/restoreState round-trips with the "Tasks" container objectName + the
    // "Tasks" inner-widget objectName preserved (the managed dock identity — R2-F3).
    void test_mount_saveStateRoundTrips_onTasksIdentity()
    {
        Gui::MainWindow* mw = ensureRealMainWindow();
        QVERIFY(mw != nullptr);
        mw->setupTaskView();
        FreeWorksGui::FwLayout::mountPropertyManager();

        QDockWidget* dock = tasksParentDock();
        QVERIFY(dock != nullptr);
        QCOMPARE(dock->objectName(), QStringLiteral("Tasks"));
        QCOMPARE(dock->widget()->objectName(), QStringLiteral("Tasks"));

        const QByteArray state = mw->saveState();
        QVERIFY2(mw->restoreState(state),
                 "saveState/restoreState must round-trip on the 'Tasks' identity");

        QDockWidget* afterDock = tasksParentDock();
        QVERIFY(afterDock != nullptr);
        QCOMPARE(afterDock->objectName(), QStringLiteral("Tasks"));
    }

    // R4-BLOCKER survival in the REAL transient order: unmountPropertyManager() FIRST
    // (it SCHEDULES the cancellable teardown), THEN signalInEdit CANCELS it, THEN drain
    // the event loop — the dock stays left and is not torn down (R3-BLOCKER2 closed).
    void test_deferredTeardown_cancelledBySignalInEdit_dockSurvives()
    {
        Gui::MainWindow* mw = ensureRealMainWindow();
        QVERIFY(mw != nullptr);
        mw->setupTaskView();
        FreeWorksGui::FwLayout::mountPropertyManager();
        QVERIFY(tasksParentDock() != nullptr);

        // The transient edit-time deactivated(): schedules, does not tear down inline.
        FreeWorksGui::FwLayout::unmountPropertyManager();
        // signalInEdit arrives within the same turn and cancels the pending teardown by
        // re-mounting (the consumer cancels + re-asserts the left placement).
        FreeWorksGui::FwLayout::mountPropertyManager();
        // Drain the queued singleShot(0): the cancel must have beaten it.
        QTest::qWait(0);
        qApp->processEvents();

        QDockWidget* dock = tasksParentDock();
        QVERIFY2(dock != nullptr,
                 "the Tasks dock must SURVIVE the transient deactivated()/re-mount (R3-ROOT)");
        QCOMPARE(mw->dockWidgetArea(dock), Qt::LeftDockWidgetArea);
    }

    // Genuine-exit case: unmountPropertyManager() with NO re-mount/signalInEdit, then
    // drain — the deferred teardown RUNS (the consumer's subscriptions drop). The dock
    // staying left is harmless under stock workbenches; we assert the teardown executed
    // by re-mount idempotency holding afterwards.
    void test_deferredTeardown_genuineExit_runs()
    {
        Gui::MainWindow* mw = ensureRealMainWindow();
        QVERIFY(mw != nullptr);
        mw->setupTaskView();
        FreeWorksGui::FwLayout::mountPropertyManager();
        QVERIFY(tasksParentDock() != nullptr);

        FreeWorksGui::FwLayout::unmountPropertyManager();
        QTest::qWait(0);
        qApp->processEvents();

        // After the teardown ran, a fresh mount still resolves the host idempotently
        // (no stale/duplicate surface left behind).
        FreeWorksGui::FwLayout::mountPropertyManager();
        QDockWidget* dock = tasksParentDock();
        QVERIFY2(dock != nullptr, "a fresh mount after a genuine-exit teardown must resolve");
        QCOMPARE(mw->dockWidgetArea(dock), Qt::LeftDockWidgetArea);
        QVERIFY2(Gui::DockWindowManager::instance()->getDockWindow("Fw_PropertyManager") == nullptr,
                 "no Fw_PropertyManager dock may remain after a re-mount");
    }
};

QTEST_MAIN(testFwPropertyManagerWidget)

#include "FwPropertyManagerWidget.moc"
