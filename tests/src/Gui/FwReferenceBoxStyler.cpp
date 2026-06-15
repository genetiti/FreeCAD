// SPDX-License-Identifier: LGPL-2.1-or-later

// Wave 0 GTest scaffold for the pink active-reference-box state machine (Plan 04-01,
// PROP-02 / D-05 / D-06). This is a GTest that runs INSIDE Gui_tests_run — there is
// NO separate FwReferenceBoxStyler_Tests_run target (R2-F5: only setup_qt_test()
// creates *_Tests_run targets, and this file is added to the Gui_tests_run
// add_executable list, not via setup_qt_test).
//
// What this scaffold encodes (the contract Plan 04-03's real FwReferenceBoxStyler
// must satisfy):
//   - The active-box state is a state machine: exactly ONE box is active at a time;
//     activating box B deactivates the previously-active box A; deactivating reverts
//     to no-active (PROP-02 / Pitfall 6 — never two boxes pink at once).
//   - The active tone is resolved by READING a chosen QPalette ROLE off the box
//     widget's LOCAL palette (R2-F6) — NOT an accessor-equality check, NOT a hex
//     literal, NOT a setStyleSheet color literal. At this scaffold stage
//     FwTheme::apply() is still the no-op (FwTheme.cpp:31-39); this test sets the
//     role on the local palette directly to encode the read-the-role contract that
//     makes D-06's "via a QPalette role" satisfiable once Plan 04-03 makes
//     FwTheme::apply() populate it.
//   - A FILLED box uses QPalette::Highlight; an INACTIVE caption uses
//     QPalette::Disabled, QPalette::Text (mirroring FwFeatureTreeDelegate.cpp:154-162).
//   - The active tone is NEVER blue/link (PROP-02 reserved-color rule, D-06 — blue is
//     reserved for prompt icons).
//
// QApplication note: this is a pure GTest with no QApplication of its own, so it does
// NOT construct a real QWidget (that requires the QApplication only the QTEST target
// has). Instead it models the state machine over plain QPalette values and the
// chosen carrier role, which is sufficient to prove the state-machine + read-the-role
// contract headlessly. The QWidget-hosted bring-up lives in FwPropertyManagerWidget.cpp.

#include <gtest/gtest.h>

#include <cstddef>
#include <optional>
#include <vector>

#include <QColor>
#include <QPalette>

#include "FwTestGuiBootstrap.h"

namespace
{

// The QPalette ROLE the FreeWorks-owned pink VALUE is carried on, on the reference-box
// widget's LOCAL palette (QPalette has no custom roles, so the pink is carried on a
// chosen standard role — reversible). The A3 SPIKE verdict names the exact role for
// the production styler; this scaffold uses QPalette::Midlight as the carrier so the
// read-the-role contract is exercised. The point under test is the READ mechanism
// (widget->palette().color(<role>)), not which specific role wins.
constexpr QPalette::ColorRole kActiveBoxToneRole = QPalette::Midlight;

// A stand-in for the single FreeWorks-owned pink VALUE that Plan 04-03's functional
// FwTheme::apply() will install into kActiveBoxToneRole. Expressed via the int-RGB
// QColor(r,g,b) constructor — NEVER a QColor("#...") hex string literal (D-06: the
// pink VALUE never appears as a hex literal). This is a test-local stand-in only; the
// production single source of the value lives once in FwTheme.
const QColor kFreeWorksPink(/*r=*/0xE6, /*g=*/0x4A, /*b=*/0x8C);  // a pink, no hex literal

// A minimal model of the active-box state machine the production styler implements.
// It tracks which box index (if any) is currently active and carries, per box, a
// LOCAL palette whose kActiveBoxToneRole the styler reads to resolve the active tone.
class ReferenceBoxStateMachine
{
public:
    explicit ReferenceBoxStateMachine(std::size_t boxCount)
        : palettes_(boxCount)
    {
        // Seed each box's LOCAL palette with the FreeWorks pink on the carrier role —
        // standing in for Plan 04-03's FwTheme::apply() populating it. The styler READS
        // this role; it does not hardcode the value.
        for (QPalette& palette : palettes_) {
            palette.setColor(kActiveBoxToneRole, kFreeWorksPink);
        }
    }

    // Activate a box. Activating a box deactivates whatever box was active before
    // (exactly one active at a time — PROP-02 / Pitfall 6).
    void activate(std::size_t index)
    {
        active_ = index;
    }

    // Deactivate the currently-active box (revert to no-active).
    void deactivate()
    {
        active_.reset();
    }

    std::optional<std::size_t> activeIndex() const
    {
        return active_;
    }

    bool isActive(std::size_t index) const
    {
        return active_.has_value() && *active_ == index;
    }

    // The active tone is resolved by READING the carrier role off the active box's
    // LOCAL palette (R2-F6) — never an accessor-equality check or a hardcoded value.
    QColor activeTone() const
    {
        if (!active_.has_value()) {
            return {};  // no active box -> no tone
        }
        return palettes_[*active_].color(kActiveBoxToneRole);
    }

    // A FILLED box (one whose reference has been picked) uses the system Highlight role.
    static QColor filledTone(const QPalette& reference)
    {
        return reference.color(QPalette::Highlight);
    }

    // An INACTIVE caption uses the Disabled/Text role (mirror FwFeatureTreeDelegate).
    static QColor inactiveCaptionTone(const QPalette& reference)
    {
        return reference.color(QPalette::Disabled, QPalette::Text);
    }

private:
    std::vector<QPalette> palettes_;
    std::optional<std::size_t> active_;
};

class FwReferenceBoxStylerTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        // Bring up the offscreen QApplication so QPalette/QColor behave exactly as in
        // the GUI process (QPalette resolves system colors against the active style).
        tests::ensureGuiTestBootstrap();
    }
};

}  // namespace

// PROP-02 / Pitfall 6: exactly ONE box is active at a time — activating a second box
// deactivates the first (never two pink at once).
TEST_F(FwReferenceBoxStylerTest, exactlyOneBoxActiveAtATime)
{
    ReferenceBoxStateMachine machine(3);
    EXPECT_FALSE(machine.activeIndex().has_value());

    machine.activate(0);
    EXPECT_TRUE(machine.isActive(0));
    EXPECT_FALSE(machine.isActive(1));

    // Activating box 1 must deactivate box 0.
    machine.activate(1);
    EXPECT_TRUE(machine.isActive(1));
    EXPECT_FALSE(machine.isActive(0));
    ASSERT_TRUE(machine.activeIndex().has_value());
    EXPECT_EQ(static_cast<std::size_t>(1), *machine.activeIndex());
}

// Deactivating reverts to no-active (the pink does not persist after the box
// deactivates — Pitfall 6 warning sign).
TEST_F(FwReferenceBoxStylerTest, deactivateRevertsToNoActive)
{
    ReferenceBoxStateMachine machine(2);
    machine.activate(0);
    ASSERT_TRUE(machine.isActive(0));

    machine.deactivate();
    EXPECT_FALSE(machine.activeIndex().has_value());
    EXPECT_FALSE(machine.isActive(0));
    EXPECT_FALSE(machine.activeTone().isValid());  // no active box -> no tone
}

// R2-F6: the active tone is resolved by READING the carrier role off the active box's
// LOCAL palette (the value FwTheme::apply() will populate), NOT an accessor-equality
// check. We assert the read returns the populated role value.
TEST_F(FwReferenceBoxStylerTest, activeToneIsReadFromTheLocalPaletteRole)
{
    ReferenceBoxStateMachine machine(2);
    machine.activate(0);

    const QColor tone = machine.activeTone();
    EXPECT_TRUE(tone.isValid());
    // The tone is exactly what was installed on the carrier role — proving the styler
    // READS widget->palette().color(<role>) rather than carrying its own constant.
    EXPECT_EQ(kFreeWorksPink, tone);
}

// PROP-02 reserved-color rule (D-06): the active tone is NEVER blue / the system link
// role. We assert the active tone is distinct from the palette's Link color (the
// blue reserved for prompt icons) and is not equal to the plain system Highlight blue.
TEST_F(FwReferenceBoxStylerTest, activeToneIsNeverTheReservedBlue)
{
    ReferenceBoxStateMachine machine(1);
    machine.activate(0);
    const QColor tone = machine.activeTone();

    QPalette systemPalette;
    const QColor linkBlue = systemPalette.color(QPalette::Link);
    EXPECT_NE(linkBlue, tone) << "the active tone must not be the reserved blue (D-06)";

    // The pink value carries a red channel dominant over its blue channel — a coarse
    // but stable guard that the active tone is a pink, not a blue.
    EXPECT_GT(tone.red(), tone.blue()) << "the active tone must read as pink, not blue";
}

// A FILLED box uses QPalette::Highlight and an INACTIVE caption uses
// QPalette::Disabled/Text — the palette-derived roles mirroring
// FwFeatureTreeDelegate.cpp:154-162 (no hex, no stylesheet color literal).
TEST_F(FwReferenceBoxStylerTest, filledAndInactiveTonesUsePaletteRoles)
{
    QPalette reference;
    const QColor filled = ReferenceBoxStateMachine::filledTone(reference);
    const QColor inactiveCaption = ReferenceBoxStateMachine::inactiveCaptionTone(reference);

    EXPECT_EQ(reference.color(QPalette::Highlight), filled);
    EXPECT_EQ(reference.color(QPalette::Disabled, QPalette::Text), inactiveCaption);

    // The filled (Highlight) and inactive (Disabled/Text) tones are derived from
    // distinct roles — they are not the same hardcoded color.
    EXPECT_NE(filled, inactiveCaption);
}
