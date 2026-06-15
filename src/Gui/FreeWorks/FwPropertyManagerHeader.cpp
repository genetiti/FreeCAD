// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 FreeWorks contributors                            *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#include <QColor>
#include <QHBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QToolButton>

#include <Gui/Control.h>

#include "FwPropertyManagerHeader.h"

using namespace FreeWorksGui;

namespace
{
// Functional accent hues (degrees on the 0-359 HSV wheel) for commit (green) and cancel
// (red). These are HUE ANGLES, not colors: the SATURATION and VALUE are taken from the
// live QPalette::Highlight role so the tone always tracks the active theme — there is NO
// hex color literal and NO setStyleSheet color literal anywhere (UI-SPEC § Color, D-06;
// mirror FwFeatureTreeDelegate's palette derivation). Green = accept/commit, red =
// cancel; blue is reserved for prompt icons and is never used here.
constexpr int kAcceptHueDeg = 120;  // green
constexpr int kCancelHueDeg = 0;    // red

/// Derive a functional tint by taking the palette Highlight role's saturation/value and
/// substituting the requested functional hue. Palette-role-derived only.
QColor functionalTintFromPalette(const QPalette& palette, int hueDeg)
{
    const QColor base = palette.color(QPalette::Active, QPalette::Highlight);
    int h = 0;
    int s = 0;
    int v = 0;
    int a = 0;
    base.getHsv(&h, &s, &v, &a);
    // Ensure the accent reads as a saturated functional tone even under a near-grey
    // theme highlight, while still tracking the theme's value (lightness) and alpha.
    const int sat = qMax(s, 160);
    return QColor::fromHsv(hueDeg, sat, v, a);
}
}  // namespace

FwPropertyManagerHeader::FwPropertyManagerHeader(QWidget* parent)
    : QWidget(parent)
{
    // Fixed 32px band (UI-SPEC § Spacing exception).
    setFixedHeight(kBandHeightPx);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 0, 4, 0);
    layout->setSpacing(4);

    // Green-✓ accept control. A QToolButton keeps the band compact; the glyph is the
    // platform-neutral check mark (no image asset, no trademark). Tooltip names the key.
    auto* accept = new QToolButton(this);
    accept->setText(QStringLiteral("✓"));  // ✓ U+2713 CHECK MARK
    accept->setToolTip(tr("Accept (Enter)"));
    accept->setAutoRaise(true);
    accept->setFocusPolicy(Qt::NoFocus);
    acceptButton_ = accept;

    // Optional panel-title label between the controls (DemiBold; Phase 7 owns fonts).
    titleLabel_ = new QLabel(this);
    QFont titleFont = titleLabel_->font();
    titleFont.setWeight(QFont::DemiBold);
    titleLabel_->setFont(titleFont);

    // Red-✗ cancel control.
    auto* cancel = new QToolButton(this);
    cancel->setText(QStringLiteral("✗"));  // ✗ U+2717 BALLOT X
    cancel->setToolTip(tr("Cancel (Esc)"));
    cancel->setAutoRaise(true);
    cancel->setFocusPolicy(Qt::NoFocus);
    cancelButton_ = cancel;

    layout->addWidget(accept);
    layout->addWidget(titleLabel_, 1);
    layout->addWidget(cancel);

    // Drive the EXISTING Control slots — NO new commit logic (D-04/D-10). These are the
    // SAME accept/reject the hosted TaskEditControl QDialogButtonBox drives
    // (TaskView.cpp:644-686).
    connect(accept, &QAbstractButton::clicked, this, [] {
        Gui::Control().accept();
    });
    connect(cancel, &QAbstractButton::clicked, this, [] {
        Gui::Control().reject();
    });

    applyPaletteTints();
}

FwPropertyManagerHeader::~FwPropertyManagerHeader() = default;

void FwPropertyManagerHeader::setTitle(const QString& title)
{
    if (titleLabel_ != nullptr) {
        titleLabel_->setText(title);
    }
}

QAbstractButton* FwPropertyManagerHeader::acceptButton() const
{
    return acceptButton_;
}

QAbstractButton* FwPropertyManagerHeader::cancelButton() const
{
    return cancelButton_;
}

void FwPropertyManagerHeader::applyPaletteTints()
{
    // Tint the two controls' foreground (ButtonText) role on their LOCAL palette with the
    // palette-derived functional green/red — native QPalette only, NO hex and NO
    // setStyleSheet color literal (D-06, UI-SPEC § Color; mirror FwFeatureTreeDelegate).
    const QPalette base = palette();

    if (acceptButton_ != nullptr) {
        QPalette p = acceptButton_->palette();
        p.setColor(QPalette::ButtonText, functionalTintFromPalette(base, kAcceptHueDeg));
        acceptButton_->setPalette(p);
    }
    if (cancelButton_ != nullptr) {
        QPalette p = cancelButton_->palette();
        p.setColor(QPalette::ButtonText, functionalTintFromPalette(base, kCancelHueDeg));
        cancelButton_->setPalette(p);
    }
}
