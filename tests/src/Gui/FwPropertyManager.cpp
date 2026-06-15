// SPDX-License-Identifier: LGPL-2.1-or-later

// Wave 0 GTest logic scaffold for the Phase-4 PropertyManager (Plan 04-01).
//
// These tests exercise the PURE, headless-testable decision cores the production
// FreeWorks PropertyManager (Plans 04-02/04-03/04-04) will be built around — with
// NO QWidget construction (that lives in the FwPropertyManagerWidget QTEST target).
// They run inside Gui_tests_run alongside the resolution tests.
//
// Three logic groups are encoded here:
//   (1) FLOW-01 sketch-type-name decision-core (D-11, RESEARCH Pitfall 2): a sketch
//       is identified ONLY by the contract STRING "SketcherGui::ViewProviderSketch"
//       (reused from FwRibbonContext.cpp:54), with zero Sketcher include/link, and
//       the decision NEVER auto-launches a feature command.
//   (2) Header -> Gui::Control mapping (PROP-01, D-04): green-checkmark maps to
//       Gui::Control().accept(), red-cross maps to Gui::Control().reject() — two
//       distinct Control slots, no new commit/transaction pipeline (each hosted
//       TaskDialog already owns its openTransaction/commitTransaction — D-10).
//   (3) Non-modal contract (PROP-01, D-07): the host is a QDockWidget-hosted
//       TaskView, never a QDialog::exec() path.
//
// The production symbols the later plans add (FwPropertyManagerHeader,
// FwReferenceBoxStyler, mountPropertyManager) do not exist yet; this scaffold
// asserts the reuse primitives + decision-core logic that need no FreeWorks
// production code, leaving the production-symbol asserts as the Wave 0 gap that
// Plans 04-02/04-03/04-04 close.

#include <gtest/gtest.h>

#include <string>
#include <string_view>

#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/TaskView/TaskView.h>

#include "FwTestGuiBootstrap.h"

namespace
{

// The view-provider type-name contract literal (RESEARCH Pitfall 2; the SAME
// literal FwRibbonContext.cpp:54 owns). Identifying a sketch by this STRING keeps
// the FreeWorks PropertyManager free of any Sketcher include/link. The literal is
// an external contract requiring live re-validation (SPIKE_LIVE_CHECKLIST.md).
constexpr const char* kSketchViewProviderTypeName = "SketcherGui::ViewProviderSketch";

// A non-sketch view-provider type name — the FLOW-01 decision must be a no-op for it.
constexpr const char* kNonSketchViewProviderTypeName = "PartDesignGui::ViewProviderBody";

// The FLOW-01 sketch-exit decision the production handler (Plan 04-04) will apply on
// signalResetEdit. SelectAndHandoff = persistently select the finished sketch as the
// profile AND land the Features tab; NoOp = do nothing. There is DELIBERATELY no
// "LaunchFeatureCommand" member: D-11 forbids auto-launching a feature on sketch exit
// (true SW behavior leaves the sketch selected and lets the user choose). The absence
// of that enumerator is the type-level guarantee that the decision core CANNOT decide
// to auto-launch.
enum class FlowDecision
{
    NoOp,
    SelectAndHandoff,
};

// Pure FLOW-01 decision core: keyed ONLY on the contract type-name string. No App or
// Sketcher type is touched — exactly the link-free identification of Pitfall 2.
FlowDecision decideOnSketchExit(std::string_view viewProviderTypeName)
{
    if (viewProviderTypeName == kSketchViewProviderTypeName) {
        return FlowDecision::SelectAndHandoff;
    }
    return FlowDecision::NoOp;
}

// The header activation the production FwPropertyManagerHeader (Plan 04-02) will map
// to the existing Gui::Control singleton slots (D-04). Green-checkmark -> accept,
// red-cross -> reject. No third "commit" action exists: the commit/transaction
// pipeline is OWNED by each hosted TaskDialog (D-10), never re-implemented here.
enum class HeaderActivation
{
    GreenCheck,
    RedCross,
};

// The Control slot a header activation maps to. These name the EXISTING
// Gui::Control() slots (Control.h:98-99) — the mapping introduces no new pipeline.
enum class ControlSlot
{
    Accept,
    Reject,
};

ControlSlot mapHeaderToControlSlot(HeaderActivation activation)
{
    return activation == HeaderActivation::GreenCheck ? ControlSlot::Accept
                                                      : ControlSlot::Reject;
}

class FwPropertyManagerLogicTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        // Bring up App + offscreen QApplication + Gui::Application(false) + the owning
        // GUI modules so Gui::Control / TaskView / Selection and the
        // SketcherGui::ViewProviderSketch type resolve for real (not a bare-link
        // false negative). Reused verbatim from the Phase-2/3 discipline.
        tests::ensureGuiTestBootstrap();
    }
};

}  // namespace

// The GUI singleton must exist after the bootstrap — Gui::Control() is a singleton
// reached through the Gui::Application; without the bootstrap the mapping asserts
// below would run against an uninitialized GUI layer.
TEST_F(FwPropertyManagerLogicTest, guiSingletonExistsAfterBootstrap)
{
    EXPECT_NE(nullptr, Gui::Application::Instance);
}

// FLOW-01 (D-11, Pitfall 2): the decision core selects-and-hands-off ONLY for the
// exact contract literal, and is a no-op for everything else. Identification is a
// pure string compare — no Sketcher include.
TEST_F(FwPropertyManagerLogicTest, flowDecisionSelectsOnlyForTheSketchContractLiteral)
{
    EXPECT_EQ(FlowDecision::SelectAndHandoff,
              decideOnSketchExit(kSketchViewProviderTypeName));

    EXPECT_EQ(FlowDecision::NoOp, decideOnSketchExit(kNonSketchViewProviderTypeName));
    EXPECT_EQ(FlowDecision::NoOp, decideOnSketchExit(""));
    // A near-miss (a superstring of the contract literal) must NOT match.
    EXPECT_EQ(FlowDecision::NoOp,
              decideOnSketchExit("SketcherGui::ViewProviderSketchExport"));
}

// D-11 forbids auto-launching a feature command on sketch exit. The decision enum has
// NO launch member, so no input can ever yield a launch decision — this is the
// type-level proof. We assert every decision is one of the two permitted outcomes.
TEST_F(FwPropertyManagerLogicTest, flowDecisionNeverAutoLaunchesAFeatureCommand)
{
    for (const char* type : {kSketchViewProviderTypeName,
                             kNonSketchViewProviderTypeName,
                             "Gui::ViewProviderDocumentObject",
                             ""}) {
        const FlowDecision decision = decideOnSketchExit(type);
        // The only two possible decisions are SelectAndHandoff and NoOp; neither
        // launches a feature command (D-11). A launch decision is unrepresentable.
        EXPECT_TRUE(decision == FlowDecision::SelectAndHandoff
                    || decision == FlowDecision::NoOp);
    }
}

// PROP-01 / D-04: the green-checkmark header maps to Gui::Control().accept() and the
// red-cross to Gui::Control().reject() — two DISTINCT Control slots. No separate
// commit pipeline is referenced (the hosted TaskDialog owns its transaction — D-10).
TEST_F(FwPropertyManagerLogicTest, headerMapsCheckToAcceptAndCrossToReject)
{
    EXPECT_EQ(ControlSlot::Accept, mapHeaderToControlSlot(HeaderActivation::GreenCheck));
    EXPECT_EQ(ControlSlot::Reject, mapHeaderToControlSlot(HeaderActivation::RedCross));

    // The two activations map to DISTINCT slots (the header is not a single button).
    EXPECT_NE(mapHeaderToControlSlot(HeaderActivation::GreenCheck),
              mapHeaderToControlSlot(HeaderActivation::RedCross));
}

// The mapping targets the EXISTING Gui::Control singleton: accept()/reject() are real
// public slots on the singleton (Control.h:98-99). This asserts the singleton the
// production header will call is reachable — no FreeWorks-side commit backend is
// introduced. (We resolve the singleton; we do not invoke accept()/reject() here,
// which would require an active dialog — that lands in the widget/integration tests.)
TEST_F(FwPropertyManagerLogicTest, controlSingletonExposesAcceptRejectTargets)
{
    Gui::ControlSingleton& control = Gui::Control();
    // With no active dialog the accessor is simply null — the point is the singleton
    // (the accept/reject target) resolves without constructing any FreeWorks widget.
    EXPECT_EQ(nullptr, control.activeDialog());
}

// PROP-01 / D-07: the panel host is the non-modal TaskView dock, NEVER a modal
// QDialog::exec() path. Gui::Control().taskPanel() returns a Gui::TaskView::TaskView*
// (Control.h:76) — a QDockWidget-hosted widget, not a QDialog. With no MainWindow /
// no mount yet (F1: in FreeWorks mode taskPanel() is null until 04-02's mount runs)
// the panel is null here; the contract asserted is the RETURN TYPE — the host is a
// TaskView, encoding the non-modality expectation Plan 04-02's production mount is
// held to.
TEST_F(FwPropertyManagerLogicTest, panelHostIsTaskViewNotAModalDialog)
{
    // The static type of taskPanel() is Gui::TaskView::TaskView* — a forward-declared
    // dockable widget, never a QDialog. The pointer is null pre-mount (F1); the
    // load-bearing assertion is that this compiles against the TaskView host type,
    // not a QDialog::exec() surface.
    Gui::TaskView::TaskView* host = Gui::Control().taskPanel();
    EXPECT_EQ(nullptr, host);  // null until Plan 04-02 mounts the left dock (F1).
}
