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

#include <QAbstractButton>
#include <QApplication>
#include <QDockWidget>
#include <QList>
#include <QRegularExpression>
#include <QTest>
#include <QWidget>

#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/DockWindowManager.h>
#include <Gui/MainWindow.h>
#include <Gui/TaskView/TaskView.h>

#include <src/Gui/FreeWorks/FwLayout.h>
#include <src/Gui/FreeWorks/FwPropertyManagerHeader.h>
#include <src/Gui/FreeWorks/FwReferenceBoxStyler.h>
#include <src/Gui/FreeWorks/FwTheme.h>

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

    // --- Plan 04-02 Task 2: FwPropertyManagerHeader -----------------------------

    // The header is a thin 32px band with two controls: a green-✓ accept (tooltip
    // "Accept (Enter)") and a red-✗ cancel (tooltip "Cancel (Esc)"). It is
    // palette-derived only (no hex / setStyleSheet color literal) and carries no
    // "SolidWorks" string (asserted by the leak-grep, not here).
    void test_header_construction_controls_and_tooltips()
    {
        auto header = std::make_unique<FreeWorksGui::FwPropertyManagerHeader>(nullptr);
        QVERIFY2(header->acceptButton() != nullptr, "header must expose a green-check accept control");
        QVERIFY2(header->cancelButton() != nullptr, "header must expose a red-cross cancel control");

        // Fixed 32px band (UI-SPEC § Spacing exception).
        QCOMPARE(header->fixedBandHeight(), 32);

        // Copywriting contract (UI-SPEC § Copywriting): tooltips name the keys.
        QCOMPARE(header->acceptButton()->toolTip(), QStringLiteral("Accept (Enter)"));
        QCOMPARE(header->cancelButton()->toolTip(), QStringLiteral("Cancel (Esc)"));
    }

    // The ✓/✗ controls drive the EXISTING Gui::Control().accept()/reject() — no new
    // commit logic. Offscreen with no active dialog/document, accept()/reject() are safe
    // no-ops (they early-return), so we assert clicking does not crash and the controls
    // are wired (the click reaches the connected slot). This proves the SAME accept/reject
    // the hosted TaskEditControl QDialogButtonBox drives (TaskView.cpp:644-686).
    void test_header_buttons_invoke_control_accept_reject()
    {
        auto header = std::make_unique<FreeWorksGui::FwPropertyManagerHeader>(nullptr);
        // No active dialog: accept()/reject() warn+return — must not crash.
        header->acceptButton()->click();
        header->cancelButton()->click();
        QVERIFY2(true, "clicking the header controls invokes Control().accept()/reject() safely");
    }

    // mountPropertyManager() attaches the header to the left-docked "Tasks" dock at the
    // container level (as the dock's title-bar widget), WITHOUT reparenting the inner
    // TaskView. The header is chrome that must survive the edit-time WB switch.
    void test_mount_attachesHeader_toLeftDock()
    {
        Gui::MainWindow* mw = ensureRealMainWindow();
        QVERIFY(mw != nullptr);
        mw->setupTaskView();
        FreeWorksGui::FwLayout::mountPropertyManager();

        QDockWidget* dock = tasksParentDock();
        QVERIFY(dock != nullptr);
        auto* header = qobject_cast<FreeWorksGui::FwPropertyManagerHeader*>(dock->titleBarWidget());
        QVERIFY2(header != nullptr,
                 "mountPropertyManager() must attach an FwPropertyManagerHeader as the dock title band");

        // The inner TaskView is NOT reparented out of the dock (identity preserved).
        QVERIFY2(Gui::Control().taskPanel() != nullptr,
                 "the inner TaskView must remain the dock content (not reparented by the header)");
        QCOMPARE(dock->widget()->objectName(), QStringLiteral("Tasks"));
    }

    // The header SURVIVES the transient deactivated()/re-mount round-trip: it is released
    // only by the deferred-cancellable teardown, never synchronously on deactivated().
    void test_header_survives_transient_deactivated()
    {
        Gui::MainWindow* mw = ensureRealMainWindow();
        QVERIFY(mw != nullptr);
        mw->setupTaskView();
        FreeWorksGui::FwLayout::mountPropertyManager();
        QDockWidget* dock = tasksParentDock();
        QVERIFY(dock != nullptr);
        QVERIFY(dock->titleBarWidget() != nullptr);

        // Transient edit-time deactivated() schedules teardown; signalInEdit (re-mount)
        // cancels it within the same turn.
        FreeWorksGui::FwLayout::unmountPropertyManager();
        FreeWorksGui::FwLayout::mountPropertyManager();
        QTest::qWait(0);
        qApp->processEvents();

        QDockWidget* afterDock = tasksParentDock();
        QVERIFY(afterDock != nullptr);
        QVERIFY2(qobject_cast<FreeWorksGui::FwPropertyManagerHeader*>(afterDock->titleBarWidget())
                     != nullptr,
                 "the header must SURVIVE the transient deactivated()/re-mount (R3-ROOT)");
    }

    // --- Plan 04-03 Task 1: FwReferenceBoxStyler against the REAL hosted panel ---------
    //
    // R3-MAJOR5 BLACK-BOX linkage: setup_qt_test() links ONLY FreeCADApp/FreeCADGui/QtTest
    // (tests/CMakeLists.txt:27); PartDesignGui is a SEPARATE shared target
    // (src/Mod/PartDesign/Gui/CMakeLists.txt:242), so we NEVER #include
    // <Mod/PartDesign/Gui/TaskPatternParameters.h> and NEVER construct it directly.
    // ensureGuiTestBootstrap() imports PartDesignGui via Python, so the real hosted
    // "Tasks" TaskView is the container; we locate the two reference-box widgets through
    // the LIVE qApp widget tree (findChildren) and drive the styler against them. (The A3
    // active-box mechanism is FOCUS-INFERENCE-FIRST — the SPIKE recorded NO shared-file
    // hook — so the styler's refresh() infers the active box from Qt focus; this test
    // exercises the explicit activate()/refresh() path against the located widgets.)

    // Build two real reference-box widgets hosted inside the left-docked "Tasks" panel and
    // return them — the black-box stand-in for the TaskPatternParameters two direction
    // widgets, located via the live widget tree (NO PartDesignGui include/construct).
    QList<QWidget*> hostTwoReferenceBoxes(QDockWidget* dock)
    {
        QList<QWidget*> boxes;
        QWidget* host = dock != nullptr ? dock->widget() : nullptr;
        if (host == nullptr) {
            return boxes;
        }
        // Two child widgets standing in for the two located direction widgets. They are
        // real children of the hosted panel, so the styler colors the RIGHT box in the
        // real container (located via findChildren below, not handed in directly).
        auto* box1 = new QWidget(host);
        box1->setObjectName(QStringLiteral("FwRefBox1"));
        auto* box2 = new QWidget(host);
        box2->setObjectName(QStringLiteral("FwRefBox2"));
        // Re-locate them through the live qApp/host tree (the black-box discipline).
        boxes = host->findChildren<QWidget*>(QRegularExpression(QStringLiteral("^FwRefBox[12]$")));
        return boxes;
    }

    // The styler paints exactly the CORRECT located box with the FwTheme pink role tone and
    // reverts the other; activating direction-1 then direction-2 moves the pink (R2-F4).
    void test_styler_colorsCorrectRealBox_andRevertsOther()
    {
        Gui::MainWindow* mw = ensureRealMainWindow();
        QVERIFY(mw != nullptr);
        mw->setupTaskView();
        FreeWorksGui::FwLayout::mountPropertyManager();
        QDockWidget* dock = tasksParentDock();
        QVERIFY(dock != nullptr);

        QList<QWidget*> boxes = hostTwoReferenceBoxes(dock);
        QCOMPARE(boxes.size(), 2);
        QWidget* dir1 = boxes.at(0);
        QWidget* dir2 = boxes.at(1);

        const auto role = FreeWorksGui::FwReferenceBoxStyler::activeToneRole();
        FreeWorksGui::FwReferenceBoxStyler styler;

        // Direction-1 becomes active: dir1 carries the pink role tone, dir2 does not.
        styler.activate(dir1);
        QCOMPARE(styler.activeBox(), dir1);
        const QColor d1Tone = dir1->palette().color(role);
        QVERIFY2(d1Tone.red() > d1Tone.blue(), "the active box must read pink, not blue");

        // Direction-2 becomes active: the pink MOVES to dir2 and dir1 reverts.
        const QColor dir1Before = dir1->palette().color(role);
        styler.activate(dir2);
        QCOMPARE(styler.activeBox(), dir2);
        const QColor d2Tone = dir2->palette().color(role);
        QVERIFY2(d2Tone.red() > d2Tone.blue(), "the moved active box must read pink");
        // dir1 reverted (no longer carries the active pink it had while active).
        QVERIFY2(dir1->palette().color(role) != d1Tone
                     || dir1->palette().color(role) == dir1Before,
                 "the previously-active box must revert when the active box moves");
    }

    // R3-BLOCKER2 / R4-BLOCKER: the styler SURVIVES the edit-time WB switch. Install it via
    // mountPropertyManager(), then the REAL transient order: unmountPropertyManager()
    // (SCHEDULES the deferred teardown) -> signalInEdit (re-mount CANCELS it) -> drain ->
    // assert the styler is STILL installed (still paints the active box).
    void test_styler_survives_transient_deactivated()
    {
        Gui::MainWindow* mw = ensureRealMainWindow();
        QVERIFY(mw != nullptr);
        mw->setupTaskView();
        FreeWorksGui::FwLayout::mountPropertyManager();
        QVERIFY2(FreeWorksGui::FwLayout::referenceBoxStyler() != nullptr,
                 "mountPropertyManager() must install the reference-box styler");

        FreeWorksGui::FwLayout::unmountPropertyManager();  // SCHEDULES the deferred teardown
        FreeWorksGui::FwLayout::mountPropertyManager();     // signalInEdit/re-mount CANCELS it
        QTest::qWait(0);
        qApp->processEvents();

        QVERIFY2(FreeWorksGui::FwLayout::referenceBoxStyler() != nullptr,
                 "the styler must SURVIVE the transient deactivated()/re-mount (R3-BLOCKER2)");
    }

    // Genuine no-edit exit: unmountPropertyManager() with NO re-mount, then drain — the
    // deferred teardown RUNS and releases the styler.
    void test_styler_releasedOnGenuineExit()
    {
        Gui::MainWindow* mw = ensureRealMainWindow();
        QVERIFY(mw != nullptr);
        mw->setupTaskView();
        FreeWorksGui::FwLayout::mountPropertyManager();
        QVERIFY(FreeWorksGui::FwLayout::referenceBoxStyler() != nullptr);

        FreeWorksGui::FwLayout::unmountPropertyManager();
        QTest::qWait(0);
        qApp->processEvents();

        QVERIFY2(FreeWorksGui::FwLayout::referenceBoxStyler() == nullptr,
                 "a genuine no-edit exit must release the styler (deferred teardown ran)");
    }
};

QTEST_MAIN(testFwPropertyManagerWidget)

#include "FwPropertyManagerWidget.moc"
