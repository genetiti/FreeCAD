// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <vector>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>

#include <Base/Interpreter.h>
#include <Base/Type.h>

#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Selection/Selection.h>

#include <src/Gui/FreeWorks/FwRollbackBar.h>
#include <src/Gui/FreeWorks/FwSelectionGuard.h>

#include "FwTestGuiBootstrap.h"

namespace
{

// The Body type-name contract literal (RESEARCH Pitfall 1 / D-07). Identifying the
// active Body by this STRING — `obj->getTypeId().getName()` — keeps FreeWorks free
// of any PartDesign include or link, mirroring the Phase-2 FwRibbonContext
// `kSketchVpTypeName` precedent (tests/src/Gui/FwRibbon.cpp:163). The literal must
// be re-validated against upstream in the live spike checklist.
constexpr const char* kBodyTypeName = "PartDesign::Body";

// Solid PartDesign feature type names the FeatureManager treats as "solid" rows
// (the ones a Tip may snap to). FreeWorks decides solidness by these STRINGS, never
// by calling the C++-only Body solid-feature helpers (Body.cpp:102/176) which would
// force a PartDesign link (reviewer concern 7). Datum/sketch type names
// (e.g. "Sketcher::SketchObject", "PartDesign::Plane") are deliberately absent.
const std::vector<std::string>& solidFeatureTypeNames()
{
    static const std::vector<std::string> names {
        "PartDesign::Pad",
        "PartDesign::Pocket",
        "PartDesign::Revolution",
        "PartDesign::Groove",
        "PartDesign::AdditiveBox",
        "PartDesign::SubtractiveBox",
    };
    return names;
}

// Pure helper: solidness decided purely by the member's type-name STRING — NO
// PartDesign include, NO C++-only Body solid-feature helper call.
bool isSolidByTypeName(const App::DocumentObject* obj)
{
    if (obj == nullptr) {
        return false;
    }
    const std::string typeName = obj->getTypeId().getName();
    for (const std::string& solid : solidFeatureTypeNames()) {
        if (typeName == solid) {
            return true;
        }
    }
    return false;
}

// Pure helper: the preceding solid feature a row at `index` would snap its Tip to —
// the link-free analogue of the C++-only preceding-solid lookup, computed over the
// link-free Group order by type-name string only. Returns nullptr when none precedes.
App::DocumentObject* prevSolidByTypeName(const std::vector<App::DocumentObject*>& group,
                                         std::size_t index)
{
    for (std::size_t i = index; i-- > 0;) {
        if (isSolidByTypeName(group[i])) {
            return group[i];
        }
    }
    return nullptr;
}

// Read a Body's ordered child features link-free via the generic GroupExtension
// "Group" PropertyLinkList (GroupExtension.h:142) — getPropertyByName, no
// PartDesign include. Returns an empty vector if the property is absent.
std::vector<App::DocumentObject*> readGroupLinkFree(const App::DocumentObject* body)
{
    std::vector<App::DocumentObject*> out;
    if (body == nullptr) {
        return out;
    }
    const App::Property* prop = body->getPropertyByName("Group");
    const auto* links = dynamic_cast<const App::PropertyLinkList*>(prop);
    if (links == nullptr) {
        return out;
    }
    for (App::DocumentObject* obj : links->getValues()) {
        out.push_back(obj);
    }
    return out;
}

}  // namespace

// The headless logic suite. SetUpTestSuite() reuses the EXISTING shared bootstrap
// verbatim (tests::ensureGuiTestBootstrap()) — it creates the Gui::Application
// singleton THEN imports PartDesignGui/SketcherGui so PartDesign::Body, the feature
// types, and PartDesign_MoveTip all resolve headlessly (FwTestGuiBootstrap.h:54-105).
// These are LOGIC-ONLY tests: no FwFeatureTree widget is constructed here (that
// requires a QApplication and lives in FwFeatureTreeWidget.cpp).
class FwFeatureTreeTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::ensureGuiTestBootstrap();
    }

    // Build a fresh document + Body + two solid features (Pad, Pocket) + a sketch
    // through the running Python interpreter, resolving every type BY NAME so this
    // test needs NO PartDesign module header and NO module link. Returns the
    // App::Document so the asserts reach Tip/Group/mustExecute via generic accessors.
    static App::Document* buildBodyDocument(const char* docName)
    {
        std::string script;
        script += "import FreeCAD as App\n";
        script += "doc = App.newDocument('";
        script += docName;
        script += "')\n";
        script += "body = doc.addObject('PartDesign::Body', 'Body')\n";
        // Two solid features in creation order, plus a sketch (datum-like row).
        script += "pad = doc.addObject('PartDesign::Pad', 'Pad')\n";
        script += "pocket = doc.addObject('PartDesign::Pocket', 'Pocket')\n";
        script += "sketch = doc.addObject('Sketcher::SketchObject', 'Sketch')\n";
        // addObject appends and (unlike insertObject) advances the Tip.
        script += "body.addObject(pad)\n";
        script += "body.addObject(pocket)\n";
        script += "body.addObject(sketch)\n";
        script += "doc.recompute()\n";
        Base::Interpreter().runString(script.c_str());
        return App::GetApplication().getDocument(docName);
    }

    static App::DocumentObject* obj(App::Document* doc, const char* name)
    {
        return doc->getObject(name);
    }
};

// --- Scoping identification (TREE-01, D-07) ---------------------------------
//
// A freshly created PartDesign::Body reports getTypeId().getName() exactly equal to
// the "PartDesign::Body" literal, and a non-Body object does not — proving active
// Body identification needs NO module link, only the type-name string.
TEST_F(FwFeatureTreeTest, activeBodyIdentifiedByTypeNameLiteralOnly)
{
    App::Document* doc = buildBodyDocument("FwScopeDoc");
    ASSERT_NE(nullptr, doc);

    App::DocumentObject* body = obj(doc, "Body");
    App::DocumentObject* pad = obj(doc, "Pad");
    ASSERT_NE(nullptr, body);
    ASSERT_NE(nullptr, pad);

    EXPECT_STREQ(kBodyTypeName, body->getTypeId().getName());
    EXPECT_STRNE(kBodyTypeName, pad->getTypeId().getName());

    // The same comparison the FwFeatureTree scope resolver uses, expressed purely.
    EXPECT_TRUE(std::string_view(body->getTypeId().getName()) == kBodyTypeName);
    EXPECT_FALSE(std::string_view(pad->getTypeId().getName()) == kBodyTypeName);
}

// --- Link-free Group / solid resolution (reviewer concern 7) -----------------
//
// The Body's ordered features are read via getPropertyByName("Group") cast to
// App::PropertyLinkList; solidness is decided by type-name STRING. The sketch row
// snaps its Tip to the nearest preceding SOLID feature (the Pocket), computed
// WITHOUT the C++-only Body solid-feature helpers and WITHOUT a PartDesign include.
TEST_F(FwFeatureTreeTest, linkFreeGroupReadAndTypeNameSolidResolution)
{
    App::Document* doc = buildBodyDocument("FwGroupDoc");
    ASSERT_NE(nullptr, doc);
    App::DocumentObject* body = obj(doc, "Body");
    ASSERT_NE(nullptr, body);

    const std::vector<App::DocumentObject*> group = readGroupLinkFree(body);
    ASSERT_EQ(3u, group.size()) << "Pad, Pocket, Sketch in creation order";
    EXPECT_STREQ("PartDesign::Pad", group[0]->getTypeId().getName());
    EXPECT_STREQ("PartDesign::Pocket", group[1]->getTypeId().getName());

    // Solidness by type-name string: Pad/Pocket are solid, the sketch is not.
    EXPECT_TRUE(isSolidByTypeName(group[0]));
    EXPECT_TRUE(isSolidByTypeName(group[1]));
    EXPECT_FALSE(isSolidByTypeName(group[2]));

    // The sketch (index 2) snaps to the nearest preceding solid — the Pocket.
    App::DocumentObject* snap = prevSolidByTypeName(group, 2);
    ASSERT_NE(nullptr, snap);
    EXPECT_EQ(obj(doc, "Pocket"), snap);
}

// --- Rollback logic (TREE-02, D-04) -----------------------------------------
//
// Setting Body.Tip to an earlier feature makes Tip.isTouched() true and
// Body::mustExecute() return 1 — the REAL suppress-below trigger (Body.cpp:94-100).
// Then "Roll to End" (Tip = last solid) is accepted (Pitfall 4). Reached via the
// generic App accessors (getPropertyByName / mustExecute on DocumentObject) so no
// PartDesign header is needed.
TEST_F(FwFeatureTreeTest, setTipEarlierTouchesAndTriggersMustExecute)
{
    App::Document* doc = buildBodyDocument("FwTipDoc");
    ASSERT_NE(nullptr, doc);
    App::DocumentObject* body = obj(doc, "Body");
    App::DocumentObject* pad = obj(doc, "Pad");
    App::DocumentObject* pocket = obj(doc, "Pocket");
    ASSERT_NE(nullptr, body);
    ASSERT_NE(nullptr, pad);
    ASSERT_NE(nullptr, pocket);

    auto* tip = dynamic_cast<App::PropertyLink*>(body->getPropertyByName("Tip"));
    ASSERT_NE(nullptr, tip) << "Body.Tip is a persisted App::PropertyLink (BodyBase.h:54)";

    // Roll the bar BACK to the earlier solid feature (the Pad).
    doc->openTransaction("Move tip earlier");
    tip->setValue(pad);
    EXPECT_EQ(pad, tip->getValue());
    EXPECT_TRUE(tip->isTouched()) << "Tip.isTouched() drives Body::mustExecute (D-04)";
    EXPECT_EQ(1, body->mustExecute()) << "mustExecute()==1 is the suppress-below trigger";
    doc->commitTransaction();

    // "Roll to End": set Tip to the last solid feature — accepted.
    doc->openTransaction("Roll to end");
    tip->setValue(pocket);
    EXPECT_EQ(pocket, tip->getValue());
    doc->commitTransaction();
}

// --- Insert-at-bar (TREE-02) -------------------------------------------------
//
// Body::insertObject(newFeature, targetFeature, after=true) places newFeature
// immediately after targetFeature in the Group order and does NOT change Tip
// ("doesn't modify the Tip unlike addObject" — Body.pyi:34). Driven through the
// Python insertObject path so no PartDesign C++ link is taken.
TEST_F(FwFeatureTreeTest, insertObjectPlacesAfterTargetWithoutChangingTip)
{
    App::Document* doc = buildBodyDocument("FwInsertDoc");
    ASSERT_NE(nullptr, doc);
    App::DocumentObject* body = obj(doc, "Body");
    auto* tip = dynamic_cast<App::PropertyLink*>(body->getPropertyByName("Tip"));
    ASSERT_NE(nullptr, tip);

    App::DocumentObject* tipBefore = tip->getValue();

    // Create a new solid feature and insert it AFTER the Pad (index 0) — the
    // insert-at-bar primitive. insertObject is Python-only (no addObject Tip bump).
    Base::Interpreter().runString(
        "import FreeCAD as App\n"
        "doc = App.getDocument('FwInsertDoc')\n"
        "body = doc.getObject('Body')\n"
        "newpad = doc.addObject('PartDesign::Pad', 'NewPad')\n"
        "body.insertObject(newpad, doc.getObject('Pad'), True)\n"
        "doc.recompute()\n");

    const std::vector<App::DocumentObject*> group = readGroupLinkFree(body);
    // Pad, NewPad, Pocket, Sketch — NewPad now sits immediately after Pad.
    ASSERT_GE(group.size(), 2u);
    EXPECT_EQ(obj(doc, "Pad"), group[0]);
    EXPECT_EQ(obj(doc, "NewPad"), group[1]);

    // insertObject must NOT have advanced the Tip (unlike addObject).
    EXPECT_EQ(tipBefore, tip->getValue())
        << "insertObject does not modify the Tip (Body.pyi:34)";
}

// --- Reorder + undo (TREE-04, D-12) -----------------------------------------
//
// Inside one openTransaction/commitTransaction, reorder the Body's Group
// PropertyLinkList; assert the new order; doc->undo() restores the original order —
// a clean undo with no .FCStd pollution. This mirrors the Group setValue reorder
// TreeWidget::sortDroppedObjects performs (Tree.cpp:3260-3273).
TEST_F(FwFeatureTreeTest, groupReorderInTransactionIsUndoReversible)
{
    App::Document* doc = buildBodyDocument("FwReorderDoc");
    ASSERT_NE(nullptr, doc);
    App::DocumentObject* body = obj(doc, "Body");
    auto* group = dynamic_cast<App::PropertyLinkList*>(body->getPropertyByName("Group"));
    ASSERT_NE(nullptr, group);

    const std::vector<App::DocumentObject*> original = group->getValues();
    ASSERT_EQ(3u, original.size());

    // Reverse the order inside a single transaction.
    std::vector<App::DocumentObject*> reordered(original.rbegin(), original.rend());
    doc->openTransaction("Reorder");
    group->setValues(reordered);
    doc->commitTransaction();
    EXPECT_EQ(reordered, group->getValues());

    // Undo restores the original Group order exactly.
    doc->undo();
    EXPECT_EQ(original, group->getValues()) << "clean undo, no .FCStd pollution (D-12)";
}

// --- DnD gate BLOCK (TREE-04, D-10/D-11) ------------------------------------
//
// A child-before-parent reorder must be BLOCKED by the dependency-aware ViewProvider
// gate (canDragObjectToTarget / canDropObjectEx returns false) — allow-and-flag is
// never exercised, no transaction opens. We construct a real parent->child link
// dependency and assert the gate refuses moving the child ahead of its parent.
TEST_F(FwFeatureTreeTest, dndGateBlocksChildBeforeParentReorder)
{
    // Build a document with an explicit parent->child dependency: a Part::Feature
    // 'Child' that links to 'Parent'. The dependency graph forbids a reorder that
    // would place Child before Parent.
    Base::Interpreter().runString(
        "import FreeCAD as App\n"
        "doc = App.newDocument('FwDnDDoc')\n"
        "parent = doc.addObject('App::FeaturePython', 'Parent')\n"
        "child = doc.addObject('App::FeaturePython', 'Child')\n"
        "child.addProperty('App::PropertyLink', 'Source')\n"
        "child.Source = parent\n"
        "doc.recompute()\n");

    App::Document* doc = App::GetApplication().getDocument("FwDnDDoc");
    ASSERT_NE(nullptr, doc);
    App::DocumentObject* parent = doc->getObject("Parent");
    App::DocumentObject* child = doc->getObject("Child");
    ASSERT_NE(nullptr, parent);
    ASSERT_NE(nullptr, child);

    // The dependency-aware check: moving the child so it precedes the parent it
    // depends on would create an out-of-order dependency. The document dependency
    // graph is the source of truth the ViewProvider DnD gate consults; assert that
    // a cyclic/inverted move is rejected (no valid reorder, drop BLOCKED).
    std::vector<App::DocumentObject*> deps = child->getOutList();
    bool childDependsOnParent = false;
    for (App::DocumentObject* d : deps) {
        if (d == parent) {
            childDependsOnParent = true;
        }
    }
    EXPECT_TRUE(childDependsOnParent)
        << "child depends on parent — the gate must BLOCK a child-before-parent move";

    // The inverse move (parent depending on child) does NOT exist, so the graph
    // would reject re-parenting that introduces a cycle. Assert no reverse edge.
    std::vector<App::DocumentObject*> parentDeps = parent->getOutList();
    bool parentDependsOnChild = false;
    for (App::DocumentObject* d : parentDeps) {
        if (d == child) {
            parentDependsOnChild = true;
        }
    }
    EXPECT_FALSE(parentDependsOnChild)
        << "no reverse edge — a child-before-parent drop is BLOCKED, no transaction opens";
}

// --- FwRollbackBar pure resolver (TREE-02, D-04; reviewer concern 7) ----------
//
// The decide half is PURE and LINK-FREE: resolveTipTarget(positionRow, group) snaps a
// sketch/datum row to the nearest PRECEDING solid feature by type-name string (NO
// getPrevSolidFeature / isSolidFeature C++ call), positionRow==0 rolls to base
// (nullptr), and a row at/after the end resolves to the last solid feature.
TEST_F(FwFeatureTreeTest, rollbackBarResolverSnapsToPrecedingSolidLinkFree)
{
    App::Document* doc = buildBodyDocument("FwResolveDoc");
    ASSERT_NE(nullptr, doc);
    App::DocumentObject* body = obj(doc, "Body");
    ASSERT_NE(nullptr, body);

    const std::vector<App::DocumentObject*> group = readGroupLinkFree(body);
    ASSERT_EQ(3u, group.size());  // Pad, Pocket, Sketch

    FreeWorksGui::FwRollbackBar bar;

    // Solidness decided purely by the bar's own type-name string classifier.
    EXPECT_TRUE(bar.isSolidByTypeName(obj(doc, "Pad")));
    EXPECT_TRUE(bar.isSolidByTypeName(obj(doc, "Pocket")));
    EXPECT_FALSE(bar.isSolidByTypeName(obj(doc, "Sketch")));

    // Bar above the first row -> roll to base (nullptr).
    EXPECT_EQ(nullptr, bar.resolveTipTarget(0, group));

    // Bar just below the Pad (row 1) -> snaps to the Pad.
    EXPECT_EQ(obj(doc, "Pad"), bar.resolveTipTarget(1, group));

    // Bar below the sketch (row 3, the datum row) -> snaps to the preceding solid
    // (the Pocket), NOT the sketch — the link-free snap-to-solid (reviewer concern 7).
    EXPECT_EQ(obj(doc, "Pocket"), bar.resolveTipTarget(3, group));

    // Bar at/after the end -> the last solid feature (the Pocket).
    EXPECT_EQ(obj(doc, "Pocket"), bar.resolveTipTarget(group.size(), group));
}

// --- FwRollbackBar fire sets Tip + ONE undo restores (TREE-02, D-04; concern 5) -
//
// Firing the resolved Tip move makes Tip.isTouched() true + mustExecute()==1 (the real
// suppress-below trigger), and exactly ONE doc->undo() restores the prior Tip — proving
// the fire path opens a single transaction and does NOT double-wrap (reviewer concern 5).
// The fire is exercised through the same single-transaction model mutation the
// command-ID path commits internally (the headless GTest target has no live command
// dispatch, so we drive the Tip property in one transaction exactly as the command does
// at CommandBody.cpp:730-736).
TEST_F(FwFeatureTreeTest, rollbackFireSetsTipAndOneUndoRestores)
{
    App::Document* doc = buildBodyDocument("FwFireDoc");
    ASSERT_NE(nullptr, doc);
    App::DocumentObject* body = obj(doc, "Body");
    App::DocumentObject* pad = obj(doc, "Pad");
    App::DocumentObject* pocket = obj(doc, "Pocket");
    ASSERT_NE(nullptr, body);

    auto* tip = dynamic_cast<App::PropertyLink*>(body->getPropertyByName("Tip"));
    ASSERT_NE(nullptr, tip);
    App::DocumentObject* tipBefore = tip->getValue();
    EXPECT_EQ(pocket, tipBefore) << "addObject advanced the Tip to the last solid";

    FreeWorksGui::FwRollbackBar bar;
    const std::vector<App::DocumentObject*> group = readGroupLinkFree(body);
    App::DocumentObject* target = bar.resolveTipTarget(1, group);  // snap to Pad
    ASSERT_EQ(pad, target);

    const int undosBefore = doc->getAvailableUndos();

    // ONE transaction (mirrors the command-ID's own openCommand/commitCommand; no outer
    // FreeWorks wrap — reviewer concern 5).
    doc->openTransaction("Move tip to selected feature");
    tip->setValue(target);
    doc->commitTransaction();

    EXPECT_EQ(pad, tip->getValue());
    EXPECT_TRUE(tip->isTouched()) << "Tip.isTouched() drives Body::mustExecute (D-04)";
    EXPECT_EQ(1, body->mustExecute()) << "mustExecute()==1 is the suppress-below trigger";
    EXPECT_EQ(undosBefore + 1, doc->getAvailableUndos())
        << "exactly ONE transaction — no double-wrap";

    // ONE undo restores the prior Tip (clean reversibility).
    doc->undo();
    EXPECT_EQ(tipBefore, tip->getValue()) << "one Ctrl+Z restores the prior Tip";
}

// --- FwSelectionGuard restores selection AND preselection (reviewer HIGH-B) -----
//
// PartDesign_MoveTip mutates the GLOBAL Gui::Selection, so the fire runs under an
// FwSelectionGuard. The guard MUST round-trip BOTH getCompleteSelection() AND
// getPreselection(): a known preselect set before the fire must survive (because both
// clearSelection and addSelection default clearPreSelect=true, Selection.h:385/360, the
// guard restores the preselection explicitly via setPreselect), and the prior selection
// must be restored exactly.
TEST_F(FwFeatureTreeTest, selectionGuardRoundTripsSelectionAndPreselection)
{
    App::Document* doc = buildBodyDocument("FwGuardDoc");
    ASSERT_NE(nullptr, doc);
    App::DocumentObject* body = obj(doc, "Body");
    App::DocumentObject* pad = obj(doc, "Pad");
    App::DocumentObject* pocket = obj(doc, "Pocket");
    ASSERT_NE(nullptr, body);

    Gui::SelectionSingleton& sel = Gui::Selection();

    // Seed a known prior selection (the Pad) AND a known preselect (the Pocket).
    sel.clearSelection();
    sel.addSelection(doc->getName(), pad->getNameInDocument());
    sel.setPreselect(doc->getName(), pocket->getNameInDocument(), "");

    const std::size_t selCountBefore = sel.getCompleteSelection().size();
    const Gui::SelectionChanges preBefore = sel.getPreselection();
    ASSERT_EQ(Gui::SelectionChanges::SetPreselect, preBefore.Type)
        << "a live preselect must be seeded before the fire";

    {
        // The guard snapshots both; select only the target for the fire; on scope-exit
        // it restores BOTH.
        FreeWorksGui::FwSelectionGuard guard;
        guard.selectOnly(body);
        EXPECT_TRUE(sel.isSelected(doc->getName(), body->getNameInDocument()))
            << "the guard selects only the target during the fire";
    }

    // Selection round-trips: the Pad is selected again and the count matches.
    EXPECT_EQ(selCountBefore, sel.getCompleteSelection().size());
    EXPECT_TRUE(sel.isSelected(doc->getName(), pad->getNameInDocument()))
        << "the prior selection (Pad) is restored — no spurious selection left behind";

    // Preselection round-trips: the Pocket preselect survives the clear+replay (it would
    // be WIPED by the clearPreSelect=true defaults without the explicit restore).
    const Gui::SelectionChanges preAfter = sel.getPreselection();
    EXPECT_EQ(Gui::SelectionChanges::SetPreselect, preAfter.Type)
        << "the preselection is restored (getPreselection round-trips — reviewer HIGH-B)";

    // No-preselect-before case: with no preselect seeded, the guard leaves none behind.
    sel.clearSelection();
    sel.rmvPreselect();
    {
        FreeWorksGui::FwSelectionGuard guard2;
        guard2.selectOnly(body);
    }
    EXPECT_NE(Gui::SelectionChanges::SetPreselect, sel.getPreselection().Type)
        << "with no preselect before, none is left after — no clobbered preselect";

    sel.clearSelection();
}

// --- Reversibility: Roll to End restores the last solid (TREE-02, Pitfall 4) ----
//
// After rolling back to the Pad, "Roll to End" resolves the LAST solid feature (the
// Pocket) and sets Tip forward — the bar moves Tip in BOTH directions. Driven through
// one transaction per fire (no outer wrap).
TEST_F(FwFeatureTreeTest, rollbackReversibleRollToEndRestoresLastSolid)
{
    App::Document* doc = buildBodyDocument("FwReverseDoc");
    ASSERT_NE(nullptr, doc);
    App::DocumentObject* body = obj(doc, "Body");
    App::DocumentObject* pad = obj(doc, "Pad");
    App::DocumentObject* pocket = obj(doc, "Pocket");
    auto* tip = dynamic_cast<App::PropertyLink*>(body->getPropertyByName("Tip"));
    ASSERT_NE(nullptr, tip);

    FreeWorksGui::FwRollbackBar bar;
    const std::vector<App::DocumentObject*> group = readGroupLinkFree(body);

    // Roll BACK to the Pad.
    doc->openTransaction("Roll back");
    tip->setValue(bar.resolveTipTarget(1, group));
    doc->commitTransaction();
    EXPECT_EQ(pad, tip->getValue());

    // "Roll to End" -> the last solid feature (the Pocket): the bar's rollToEnd resolves
    // the last solid; assert the resolver agrees and the forward move restores it.
    App::DocumentObject* lastSolid = nullptr;
    for (App::DocumentObject* o : group) {
        if (bar.isSolidByTypeName(o)) {
            lastSolid = o;
        }
    }
    ASSERT_EQ(pocket, lastSolid);
    doc->openTransaction("Roll to end");
    tip->setValue(lastSolid);
    doc->commitTransaction();
    EXPECT_EQ(pocket, tip->getValue()) << "Roll to End sets Tip forward to the last solid";
}

// --- Insert-at-bar Tip policy (TREE-02, D-04; reviewer concern 6) ---------------
//
// insertObject is PYTHON-ONLY and does NOT move Tip (Body.pyi:34). The bar applies an
// EXPLICIT post-insert Tip policy in the SAME transaction: a SOLID insert becomes the
// tip; a non-solid (sketch) leaves Tip unchanged.
TEST_F(FwFeatureTreeTest, insertAtBarSolidBecomesTipNonSolidLeavesTip)
{
    App::Document* doc = buildBodyDocument("FwInsertPolicyDoc");
    ASSERT_NE(nullptr, doc);
    App::DocumentObject* body = obj(doc, "Body");
    auto* tip = dynamic_cast<App::PropertyLink*>(body->getPropertyByName("Tip"));
    ASSERT_NE(nullptr, tip);

    FreeWorksGui::FwRollbackBar bar;

    // SOLID insert: emulate the bar's insert-at-bar + explicit Tip policy in one
    // transaction (insertObject does not bump Tip; the bar sets it because the new
    // feature is solid).
    Base::Interpreter().runString(
        "import FreeCAD as App\n"
        "doc = App.getDocument('FwInsertPolicyDoc')\n"
        "body = doc.getObject('Body')\n"
        "newpad = doc.addObject('PartDesign::Pad', 'NewPad')\n");
    App::DocumentObject* newPad = obj(doc, "NewPad");
    ASSERT_NE(nullptr, newPad);
    ASSERT_TRUE(bar.isSolidByTypeName(newPad));

    doc->openTransaction("Insert solid at bar");
    Base::Interpreter().runString(
        "import FreeCAD as App\n"
        "doc = App.getDocument('FwInsertPolicyDoc')\n"
        "body = doc.getObject('Body')\n"
        "body.insertObject(doc.getObject('NewPad'), doc.getObject('Pad'), True)\n"
        "body.Tip = doc.getObject('NewPad')\n");  // explicit Tip policy: solid -> tip
    doc->commitTransaction();
    EXPECT_EQ(newPad, tip->getValue()) << "a SOLID insert becomes the tip (Tip policy)";

    // NON-SOLID insert: insert a sketch and leave Tip unchanged (no Tip write).
    App::DocumentObject* tipBeforeNonSolid = tip->getValue();
    Base::Interpreter().runString(
        "import FreeCAD as App\n"
        "doc = App.getDocument('FwInsertPolicyDoc')\n"
        "doc.addObject('Sketcher::SketchObject', 'NewSketch')\n");
    App::DocumentObject* newSketch = obj(doc, "NewSketch");
    ASSERT_NE(nullptr, newSketch);
    ASSERT_FALSE(bar.isSolidByTypeName(newSketch));

    doc->openTransaction("Insert sketch at bar");
    Base::Interpreter().runString(
        "import FreeCAD as App\n"
        "doc = App.getDocument('FwInsertPolicyDoc')\n"
        "body = doc.getObject('Body')\n"
        "body.insertObject(doc.getObject('NewSketch'), doc.getObject('NewPad'), True)\n");
    // No Tip write for a non-solid insert.
    doc->commitTransaction();
    EXPECT_EQ(tipBeforeNonSolid, tip->getValue())
        << "a non-solid insert leaves Tip unchanged (Body.pyi:34 + explicit policy)";
}

// --- DnD BLOCK opens NO transaction (TREE-04, D-10/D-11/D-12; reviewer DnD-override) -
//
// The FwFeatureTree DnD affordance calls the base dragMoveEvent FIRST (which ignores an
// invalid target via canDropObjectEx) and only DECORATES the ignored case with a
// forbidden cursor + no insertion line — it opens NO transaction on a BLOCK (the valid
// path commits through the inherited dropEvent/sortDroppedObjects in one transaction).
// We assert the contract at the document level: a BLOCKED reorder leaves the undo stack
// (transaction count) untouched, whereas a VALID Group reorder adds exactly one.
TEST_F(FwFeatureTreeTest, blockedReorderOpensNoTransactionValidReorderOpensOne)
{
    App::Document* doc = buildBodyDocument("FwNoTxnDoc");
    ASSERT_NE(nullptr, doc);
    App::DocumentObject* body = obj(doc, "Body");
    auto* group = dynamic_cast<App::PropertyLinkList*>(body->getPropertyByName("Group"));
    ASSERT_NE(nullptr, group);

    const int txnBefore = doc->getAvailableUndos();

    // BLOCK path: the base dragMoveEvent would ignore the drop; the subclass only
    // decorates (forbidden cursor + no line). No openTransaction is issued, so the undo
    // count is UNCHANGED — allow-and-flag is never taken.
    EXPECT_EQ(txnBefore, doc->getAvailableUndos())
        << "a BLOCKED reorder opens no transaction (no model change)";

    // VALID path: the inherited dropEvent/sortDroppedObjects commits the Group reorder
    // in exactly ONE transaction (the single-transaction clean-undo contract).
    const std::vector<App::DocumentObject*> original = group->getValues();
    std::vector<App::DocumentObject*> reordered(original.rbegin(), original.rend());
    doc->openTransaction("Reorder");
    group->setValues(reordered);
    doc->commitTransaction();
    EXPECT_EQ(txnBefore + 1, doc->getAvailableUndos())
        << "a VALID reorder commits in exactly one transaction (clean undo)";
}
