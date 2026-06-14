// SPDX-License-Identifier: LGPL-2.1-or-later

#include <string>
#include <vector>

#include <QAction>
#include <QKeySequence>
#include <QList>
#include <QTest>
#include <QTreeWidgetItem>
#include <QWidget>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>

#include <Base/Interpreter.h>

#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Tree.h>
#include <Gui/ViewProviderDocumentObject.h>

#include <src/Gui/FreeWorks/FwFeatureTree.h>

#include "FwTestGuiBootstrap.h"

namespace
{
constexpr const char* kBodyTypeName = "PartDesign::Body";

// Ensure a real Gui::MainWindow is the registered FreeCAD singleton — the
// FwFeatureTree, like the stock Gui::TreeWidget, reaches the active document and
// view providers through Gui::Application/Gui::getMainWindow(). Constructing
// Gui::MainWindow sets MainWindow::instance = this (MainWindow.cpp:358). Leaked
// deliberately so it outlives the QTEST_MAIN process (mirrors FwRibbonWidget.cpp).
Gui::MainWindow* ensureRealMainWindow()
{
    if (Gui::getMainWindow() == nullptr) {
        new Gui::MainWindow();  // sets MainWindow::instance = this
    }
    return Gui::getMainWindow();
}

// Resolve the App object behind a tree item via the PUBLIC DocumentObjectItem
// accessor (Tree.h:495) — the same surface FwFeatureTree's scope render uses. Returns
// nullptr for a DocumentItem (no underlying App object) or any non-object item.
App::DocumentObject* objectOfItem(QTreeWidgetItem* item)
{
    auto* objItem = dynamic_cast<Gui::DocumentObjectItem*>(item);
    if (objItem == nullptr) {
        return nullptr;
    }
    Gui::ViewProviderDocumentObject* vp = objItem->object();
    return vp != nullptr ? vp->getObject() : nullptr;
}

// Find the tree item whose underlying App object has the given internal Name, by
// descending the REAL nested topology from invisibleRootItem() (DocumentItem ->
// Body -> features) — never by treating a Body as a top-level row, since a Body is a
// CHILD of the per-document DocumentItem, not a top-level item (Tree.cpp:4562).
QTreeWidgetItem* findItemByObjectName(QTreeWidgetItem* root, const std::string& name)
{
    for (int i = 0; i < root->childCount(); ++i) {
        QTreeWidgetItem* child = root->child(i);
        App::DocumentObject* o = objectOfItem(child);
        if (o != nullptr && name == o->getNameInDocument()) {
            return child;
        }
        if (QTreeWidgetItem* found = findItemByObjectName(child, name)) {
            return found;
        }
    }
    return nullptr;
}
}  // namespace

// FwFeatureTree is a QWidget (Gui::TreeWidget subclass); constructing it REQUIRES a
// live QApplication, which the plain Gui_tests_run GTest target lacks. So the
// construction + F2-action + REAL two-Body scoping-render assertions live here in a
// QTEST_MAIN Qt target (registered via setup_qt_test, QT_QPA_PLATFORM=offscreen),
// mirroring the Phase-2 FwRibbonWidget split.
class testFwFeatureTreeWidget: public QObject
{
    Q_OBJECT

public:
    testFwFeatureTreeWidget()
    {
        // App::Application + QApplication + Gui::Application singleton + the owning
        // GUI modules, so PartDesign::Body and the feature types resolve.
        tests::ensureGuiTestBootstrap();
    }

private Q_SLOTS:

    void init()
    {}

    void cleanup()
    {}

    // TREE-01 / D-13: an FwFeatureTree constructs under the QTEST QApplication, is-a
    // Gui::TreeWidget, and carries the inherited F2 rename action (the action whose
    // shortcut is Qt::Key_F2, or Qt::Key_Return on macOS).
    void test_ConstructsAsTreeWidgetWithF2Action()
    {
        auto tree = std::make_unique<FreeWorksGui::FwFeatureTree>("FwFeatureManager", nullptr);
        QVERIFY2(dynamic_cast<Gui::TreeWidget*>(tree.get()) != nullptr,
                 "FwFeatureTree must be-a Gui::TreeWidget (thin subclass, not from-scratch)");

        bool hasRenameKey = false;
        for (QAction* act : tree->actions()) {
            const QKeySequence seq = act->shortcut();
            if (seq == QKeySequence(Qt::Key_F2) || seq == QKeySequence(Qt::Key_Return)) {
                hasRenameKey = true;
                break;
            }
        }
        QVERIFY2(hasRenameKey,
                 "the inherited F2 (or macOS Return) rename action must be present (D-13)");
    }

    // TREE-01 / D-03 item 2 — the scoping RENDER over the REAL nested topology.
    //
    // Build a real document with TWO PartDesign::Body objects, each carrying an Origin
    // + a solid feature, so the stock Gui::DocumentItem is the top-level item and the
    // two Bodies are nested CHILDREN under it (Tree.cpp:4562). Activate ONE Body, call
    // scopeToActiveBody(), then assert the recursive setHidden render:
    //   - the ancestor DocumentItem is NOT hidden,
    //   - the active Body's item is NOT hidden,
    //   - the NON-active Body's item IS hidden (isHidden()==true),
    //   - the active Body's Origin is the FIRST visible child under the active Body.
    // This is the REAL-DOM proof for the spike item-2 PASS — NOT two synthetic
    // top-level QTreeWidgetItems (reviewer HIGH-A).
    void test_ScopeToActiveBodyHidesNonActiveBodySubtree()
    {
        ensureRealMainWindow();

        // Two Bodies, each with an Origin (addObject creates the Origin) + a Pad.
        Base::Interpreter().runString(
            "import FreeCAD as App\n"
            "import FreeCADGui as Gui\n"
            "doc = App.newDocument('FwTwoBodyDoc')\n"
            "Gui.activeDocument()\n"
            "bodyA = doc.addObject('PartDesign::Body', 'BodyA')\n"
            "padA = doc.addObject('PartDesign::Pad', 'PadA')\n"
            "bodyA.addObject(padA)\n"
            "bodyB = doc.addObject('PartDesign::Body', 'BodyB')\n"
            "padB = doc.addObject('PartDesign::Pad', 'PadB')\n"
            "bodyB.addObject(padB)\n"
            "doc.recompute()\n");

        App::Document* appDoc = App::GetApplication().getDocument("FwTwoBodyDoc");
        QVERIFY2(appDoc != nullptr, "the two-Body document must exist");
        App::DocumentObject* bodyA = appDoc->getObject("BodyA");
        App::DocumentObject* bodyB = appDoc->getObject("BodyB");
        QVERIFY(bodyA != nullptr && bodyB != nullptr);
        QCOMPARE(QString(bodyA->getTypeId().getName()), QString(kBodyTypeName));

        // Make the FwFeatureTree show this document so the stock DocumentItem is the
        // top-level item and the two Bodies are nested children beneath it.
        auto tree = std::make_unique<FreeWorksGui::FwFeatureTree>("FwFeatureManager", nullptr);
        Gui::Document* guiDoc = Gui::Application::Instance->getDocument(appDoc);
        QVERIFY2(guiDoc != nullptr, "a Gui::Document must mirror the App::Document");
        tree->setDocument(guiDoc);

        // Activate BodyA, then run the scoping render.
        tree->setActiveBody(bodyA);
        tree->scopeToActiveBody();

        QTreeWidgetItem* root = tree->invisibleRootItem();
        QTreeWidgetItem* itemA = findItemByObjectName(root, "BodyA");
        QTreeWidgetItem* itemB = findItemByObjectName(root, "BodyB");
        QVERIFY2(itemA != nullptr, "BodyA must have a tree item (nested under the DocumentItem)");
        QVERIFY2(itemB != nullptr, "BodyB must have a tree item (nested under the DocumentItem)");

        // The DocumentItem ancestor of BodyA stays visible.
        QTreeWidgetItem* docItem = itemA->parent();
        QVERIFY2(docItem != nullptr, "the Body must be a CHILD of the DocumentItem, not top-level");
        QVERIFY2(dynamic_cast<Gui::DocumentItem*>(docItem) != nullptr,
                 "the Body's parent must be the stock Gui::DocumentItem (real nested topology)");
        QVERIFY2(!docItem->isHidden(), "the ancestor DocumentItem must stay visible");

        // The active Body's subtree is shown; the non-active Body's subtree is hidden.
        QVERIFY2(!itemA->isHidden(), "the active Body subtree must be visible");
        QVERIFY2(itemB->isHidden(),
                 "the NON-active Body subtree must be hidden via setHidden(true)");

        // The active Body's Origin renders FIRST among its visible (non-hidden) children.
        QTreeWidgetItem* firstVisibleChild = nullptr;
        for (int i = 0; i < itemA->childCount(); ++i) {
            if (!itemA->child(i)->isHidden()) {
                firstVisibleChild = itemA->child(i);
                break;
            }
        }
        QVERIFY2(firstVisibleChild != nullptr, "the active Body must show at least one child");
        App::DocumentObject* firstObj = objectOfItem(firstVisibleChild);
        QVERIFY2(firstObj != nullptr, "the first visible child must resolve to an App object");
        const QString firstType(firstObj->getTypeId().getName());
        QVERIFY2(firstType.contains(QStringLiteral("Origin")),
                 "the active Body's Origin must render first among the visible rows");
    }
};

QTEST_MAIN(testFwFeatureTreeWidget)

#include "FwFeatureTreeWidget.moc"
