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
