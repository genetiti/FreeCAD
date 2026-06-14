// SPDX-License-Identifier: LGPL-2.1-or-later

#include <string>
#include <vector>

#include <QAction>
#include <QEvent>
#include <QImage>
#include <QKeySequence>
#include <QList>
#include <QModelIndex>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QTest>
#include <QTreeWidgetItem>
#include <QWidget>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/PropertyStandard.h>

#include <Base/Interpreter.h>

#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Tree.h>
#include <Gui/ViewProviderDocumentObject.h>

#include <src/Gui/FreeWorks/FwFeatureTree.h>
#include <src/Gui/FreeWorks/FwFeatureTreeDelegate.h>

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

// Find the first tree item whose underlying App object carries the given internal
// "Role" PropertyString value (e.g. "XY_Plane"), descending the REAL nested topology
// — the same generic-accessor path the delegate uses to resolve the plane role.
QTreeWidgetItem* findItemByRole(QTreeWidgetItem* root, const std::string& roleValue)
{
    for (int i = 0; i < root->childCount(); ++i) {
        QTreeWidgetItem* child = root->child(i);
        App::DocumentObject* o = objectOfItem(child);
        if (o != nullptr) {
            const App::Property* prop = o->getPropertyByName("Role");
            const auto* roleProp = dynamic_cast<const App::PropertyString*>(prop);
            if (roleProp != nullptr && roleValue == roleProp->getValue()) {
                return child;
            }
        }
        if (QTreeWidgetItem* found = findItemByRole(child, roleValue)) {
            return found;
        }
    }
    return nullptr;
}

// Expose the protected display-text remap so the QTEST can assert the rendered text
// without painting to a surface (initStyleOption is the display-only seam the delegate
// overrides; option.text after it holds exactly what the row would render).
class ProbeDelegate: public FreeWorksGui::FwFeatureTreeDelegate
{
public:
    explicit ProbeDelegate(QObject* parent)
        : FreeWorksGui::FwFeatureTreeDelegate(parent)
    {}
    QString renderedText(const QModelIndex& index) const
    {
        QStyleOptionViewItem opt;
        initStyleOption(&opt, index);
        return opt.text;
    }
};

// Expose the protected rollback-band seams so the QTEST can assert the band paint and
// the grab-zone hit-test without a live pointer-drag.
class ProbeTree: public FreeWorksGui::FwFeatureTree
{
public:
    explicit ProbeTree(const char* name, QWidget* parent)
        : FreeWorksGui::FwFeatureTree(name, parent)
    {}
    int probeTipBoundaryY() const
    {
        return tipBoundaryY();
    }
    bool probeIsWithinGrabZone(int y) const
    {
        return isWithinGrabZone(y);
    }
    // Drive the hover affordance directly and report the resulting cursor shape so the
    // QTEST can assert Qt::SizeVerCursor over the grab zone without a live mouse.
    Qt::CursorShape probeHoverCursorAt(int y)
    {
        QMouseEvent move(QEvent::MouseMove,
                         QPointF(4, y),
                         QPointF(4, y),
                         Qt::NoButton,
                         Qt::NoButton,
                         Qt::NoModifier);
        mouseMoveEvent(&move);
        return cursor().shape();
    }
    // Paint a single row through the overridden drawRow and report whether the band
    // (a horizontal Highlight-coloured strip) appears at the given row rect.
    void probeDrawRow(QPainter* painter,
                      const QStyleOptionViewItem& option,
                      const QModelIndex& index) const
    {
        drawRow(painter, option, index);
    }
};
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

    // TREE-01 — after construction the tree has the FwFeatureTreeDelegate installed and
    // the UI-SPEC 16px indent metric pinned, and reports the 'No active Body' empty
    // state when no Body has been activated.
    void test_DelegateInstalledIndent16AndEmptyState()
    {
        auto tree = std::make_unique<FreeWorksGui::FwFeatureTree>("FwFeatureManager", nullptr);

        QVERIFY2(dynamic_cast<FreeWorksGui::FwFeatureTreeDelegate*>(tree->itemDelegate())
                     != nullptr,
                 "the FwFeatureTreeDelegate must be installed via setItemDelegate");
        QCOMPARE(tree->indentation(), 16);

        // No Body activated yet -> empty state, with the exact UI-SPEC copy.
        QVERIFY2(tree->isEmptyState(), "a freshly built tree with no active Body is empty-state");
        QCOMPARE(FreeWorksGui::FwFeatureTree::noActiveBodyText(), QStringLiteral("No active Body"));
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

    // TREE-01 / D-08 — the FwFeatureTreeDelegate plane DISPLAY-NAME remap, resolved
    // through the STOCK tree item-object path (no fabricated model).
    //
    // Build a real Body whose Origin owns the three stock planes (each an App::Plane
    // DatumElement carrying a "Role" PropertyString of XY_Plane / XZ_Plane / YZ_Plane).
    // Install the delegate on the tree, then assert the RENDERED display text is
    // "Front Plane" / "Top Plane" / "Right Plane" — and that the underlying objects'
    // Label values are UNCHANGED (display-only remap, the object is never mutated).
    void test_DelegateRemapsPlaneDisplayNamesThroughStockItemPath()
    {
        ensureRealMainWindow();

        Base::Interpreter().runString(
            "import FreeCAD as App\n"
            "import FreeCADGui as Gui\n"
            "doc = App.newDocument('FwPlaneRemapDoc')\n"
            "Gui.activeDocument()\n"
            "body = doc.addObject('PartDesign::Body', 'Body')\n"
            "doc.recompute()\n");

        App::Document* appDoc = App::GetApplication().getDocument("FwPlaneRemapDoc");
        QVERIFY2(appDoc != nullptr, "the plane-remap document must exist");

        auto tree = std::make_unique<FreeWorksGui::FwFeatureTree>("FwFeatureManager", nullptr);
        Gui::Document* guiDoc = Gui::Application::Instance->getDocument(appDoc);
        QVERIFY2(guiDoc != nullptr, "a Gui::Document must mirror the App::Document");
        tree->setDocument(guiDoc);

        // The delegate is parented to the tree so it resolves rows via the tree's
        // itemFromIndex() — the stock item-object path.
        auto* probe = new ProbeDelegate(tree.get());

        QTreeWidgetItem* root = tree->invisibleRootItem();
        struct Expectation
        {
            const char* role;
            QString display;
        };
        const Expectation cases[3] = {
            {"XY_Plane", FreeWorksGui::FwFeatureTreeDelegate::frontPlaneName()},
            {"XZ_Plane", FreeWorksGui::FwFeatureTreeDelegate::topPlaneName()},
            {"YZ_Plane", FreeWorksGui::FwFeatureTreeDelegate::rightPlaneName()},
        };

        for (const Expectation& exp : cases) {
            QTreeWidgetItem* item = findItemByRole(root, exp.role);
            QVERIFY2(item != nullptr, "each origin plane must have a tree item (stock topology)");
            App::DocumentObject* planeObj = objectOfItem(item);
            QVERIFY2(planeObj != nullptr, "the plane row must resolve to an App object");
            const std::string labelBefore = planeObj->Label.getValue();

            const QModelIndex index = tree->indexFromItem(item);
            const QString rendered = probe->renderedText(index);
            QCOMPARE(rendered, exp.display);

            // The underlying Label is NEVER written by the display-only remap (D-08).
            QCOMPARE(QString::fromStdString(planeObj->Label.getValue()),
                     QString::fromStdString(labelBefore));
        }

        // A non-plane row (the Body itself) is a pass-through: its rendered text is the
        // stock Label, never one of the plane display names.
        QTreeWidgetItem* bodyItem = findItemByObjectName(root, "Body");
        QVERIFY2(bodyItem != nullptr, "the Body must have a tree item");
        const QString bodyRendered = probe->renderedText(tree->indexFromItem(bodyItem));
        QVERIFY2(bodyRendered != FreeWorksGui::FwFeatureTreeDelegate::frontPlaneName(),
                 "a non-plane row must pass through (never a plane display name)");
    }

    // TREE-02 — the rollback band renders at the tip boundary, the grab zone reports
    // Qt::SizeVerCursor, the below-tip rows carry the greyed kBelowTipRole flag (no
    // show/hide write), and the three Roll context actions exist with the exact labels.
    void test_RollbackBandRendersGreysBelowTipAndRollActions()
    {
        ensureRealMainWindow();

        // A Body with two solid features; addObject advances the Tip to the last (Pad2).
        Base::Interpreter().runString(
            "import FreeCAD as App\n"
            "import FreeCADGui as Gui\n"
            "doc = App.newDocument('FwRollbackUiDoc')\n"
            "Gui.activeDocument()\n"
            "body = doc.addObject('PartDesign::Body', 'Body')\n"
            "pad1 = doc.addObject('PartDesign::Pad', 'Pad1')\n"
            "pad2 = doc.addObject('PartDesign::Pad', 'Pad2')\n"
            "body.addObject(pad1)\n"
            "body.addObject(pad2)\n"
            "doc.recompute()\n"
            "body.Tip = pad1\n"  // roll the tip back to Pad1 -> Pad2 is below-tip
            "doc.recompute()\n");

        App::Document* appDoc = App::GetApplication().getDocument("FwRollbackUiDoc");
        QVERIFY2(appDoc != nullptr, "the rollback-ui document must exist");
        App::DocumentObject* body = appDoc->getObject("Body");
        App::DocumentObject* pad1 = appDoc->getObject("Pad1");
        App::DocumentObject* pad2 = appDoc->getObject("Pad2");
        QVERIFY(body != nullptr && pad1 != nullptr && pad2 != nullptr);

        auto tree = std::make_unique<ProbeTree>("FwFeatureManager", nullptr);
        Gui::Document* guiDoc = Gui::Application::Instance->getDocument(appDoc);
        QVERIFY2(guiDoc != nullptr, "a Gui::Document must mirror the App::Document");
        tree->setDocument(guiDoc);
        tree->setActiveBody(body);
        tree->scopeToActiveBody();
        tree->resize(240, 320);
        tree->show();
        QVERIFY(QTest::qWaitForWindowExposed(tree.get()));

        // The tip is Pad1; the band boundary is the bottom edge of the Pad1 row.
        QCOMPARE(tree->currentTipFeature(), pad1);
        const int boundary = tree->probeTipBoundaryY();
        QVERIFY2(boundary >= 0, "the tip boundary must resolve to a laid-out row");

        // Grab zone: a y within the 8px zone centred on the boundary reports
        // Qt::SizeVerCursor (vertical draggability), and a y far away does not.
        QVERIFY2(tree->probeIsWithinGrabZone(boundary),
                 "the band centre must be inside the grab zone");
        QCOMPARE(tree->probeHoverCursorAt(boundary), Qt::SizeVerCursor);
        QVERIFY2(!tree->probeIsWithinGrabZone(boundary + 100),
                 "a point far from the band must be outside the grab zone");

        // Below-tip greying: after refresh, the row AFTER the tip (Pad2) carries the
        // Gui-only kBelowTipRole flag; the tip row (Pad1) does not. No show/hide write.
        tree->refreshBelowTipGreying();
        QTreeWidgetItem* root = tree->invisibleRootItem();
        QTreeWidgetItem* pad1Item = findItemByObjectName(root, "Pad1");
        QTreeWidgetItem* pad2Item = findItemByObjectName(root, "Pad2");
        QVERIFY(pad1Item != nullptr && pad2Item != nullptr);
        const QVariant pad2Flag =
            pad2Item->data(0, FreeWorksGui::FwFeatureTreeDelegate::kBelowTipRole);
        const QVariant pad1Flag =
            pad1Item->data(0, FreeWorksGui::FwFeatureTreeDelegate::kBelowTipRole);
        QVERIFY2(pad2Flag.toBool(), "the row below the tip (Pad2) must carry the below-tip role");
        QVERIFY2(!pad1Flag.toBool(), "the tip row (Pad1) must NOT be greyed");

        // The band paints at the tip-boundary row: render the Pad1 row through drawRow
        // into an image and assert a Highlight-coloured horizontal strip appears at the
        // band thickness near the row bottom.
        const QRect rowRect = tree->visualItemRect(pad1Item);
        QVERIFY(rowRect.isValid());
        QImage image(tree->viewport()->width(), rowRect.height() + 8, QImage::Format_ARGB32);
        image.fill(Qt::transparent);
        {
            QPainter painter(&image);
            QStyleOptionViewItem opt;
            opt.rect = QRect(0, 0, tree->viewport()->width(), rowRect.height());
            // Offset so opt.rect.bottom() matches the tip boundary the band keys on.
            opt.rect.moveBottom(boundary);
            opt.palette = tree->palette();
            const QModelIndex index = tree->indexFromItem(pad1Item);
            tree->probeDrawRow(&painter, opt, index);
        }
        const QColor highlight = tree->palette().color(QPalette::Highlight);
        bool bandPainted = false;
        for (int yy = 0; yy < image.height() && !bandPainted; ++yy) {
            for (int xx = 0; xx < image.width(); ++xx) {
                if (image.pixelColor(xx, yy) == highlight) {
                    bandPainted = true;
                    break;
                }
            }
        }
        QVERIFY2(bandPainted, "the 4px Highlight rollback band must paint at the tip boundary");

        // The three Roll context actions carry the exact UI-SPEC labels.
        QCOMPARE(FreeWorksGui::FwFeatureTree::rollBackLabel(), QStringLiteral("Roll Back"));
        QCOMPARE(FreeWorksGui::FwFeatureTree::rollForwardLabel(), QStringLiteral("Roll Forward"));
        QCOMPARE(FreeWorksGui::FwFeatureTree::rollToEndLabel(), QStringLiteral("Roll to End"));
        QCOMPARE(FreeWorksGui::FwFeatureTree::rollbackBarTooltip(),
                 QStringLiteral("Drag to roll the model back or forward"));
    }
};

QTEST_MAIN(testFwFeatureTreeWidget)

#include "FwFeatureTreeWidget.moc"
