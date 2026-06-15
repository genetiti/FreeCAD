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

#pragma once

#include "PreCompiled.h"

#include <QWidget>

class QAbstractButton;
class QLabel;

namespace FreeWorksGui
{

/**
 * The thin PropertyManager header band (PROP-01): a fixed 32px chrome strip with a
 * green-✓ accept control and a red-✗ cancel control, plus an optional panel-title label
 * between them. It mirrors the FwRollbackBar / FwRibbonContext discipline (a small
 * FreeWorks chrome widget holding only weak references and NO persisted state).
 *
 * The ✓ control calls Gui::Control().accept() and the ✗ control calls
 * Gui::Control().reject() (Control.h:98-100) — the SAME accept/reject the hosted
 * TaskEditControl QDialogButtonBox drives (TaskView.cpp:644-686). The header introduces
 * NO new commit/transaction logic: each hosted TaskDialog already wraps
 * openTransaction/commitTransaction (D-04/D-10).
 *
 * Color is QPalette-ROLE-derived ONLY (the green/red functional tones are HSV-shifted
 * from QPalette::Highlight; mirror FwFeatureTreeDelegate.cpp:154-162) — there is NO hex
 * literal and NO setStyleSheet color literal anywhere, and NO trademark identifier
 * string (UI-SPEC § Color / § Copywriting; LOCKED fork rule). Tooltips read
 * "Accept (Enter)" / "Cancel (Esc)".
 *
 * FwLayout::mountPropertyManager() attaches this header to the left-docked "Tasks" dock
 * at the CONTAINER level (the dock's title-bar widget) WITHOUT reparenting the inner
 * TaskView. Because it is chrome that must SURVIVE the edit-time workbench switch
 * (R3-ROOT), it is installed by mountPropertyManager() and released ONLY by the deferred-
 * cancellable teardown — never synchronously on the transient deactivated().
 *
 * Compiled directly into FreeCADGui and used in-process (mirrors FwRibbon /
 * FwFeatureTree), so no cross-library export macro is required.
 */
class FwPropertyManagerHeader: public QWidget
{
    Q_OBJECT

public:
    /// The fixed band height in pixels (UI-SPEC § Spacing exception).
    static constexpr int kBandHeightPx = 32;

    explicit FwPropertyManagerHeader(QWidget* parent = nullptr);
    ~FwPropertyManagerHeader() override;

    /// Set the panel title shown between the two controls (DemiBold; Phase 7 owns fonts).
    void setTitle(const QString& title);

    /// The green-✓ accept control (drives Gui::Control().accept()). Never null.
    QAbstractButton* acceptButton() const;
    /// The red-✗ cancel control (drives Gui::Control().reject()). Never null.
    QAbstractButton* cancelButton() const;

    /// The fixed band height the widget enforces (test/seam accessor).
    int fixedBandHeight() const
    {
        return kBandHeightPx;
    }

private:
    /// Apply the QPalette-role-derived green/red tints to the two controls (NO hex / NO
    /// setStyleSheet color literal). Re-run on palette changes so it follows the theme.
    void applyPaletteTints();

    QAbstractButton* acceptButton_ = nullptr;
    QAbstractButton* cancelButton_ = nullptr;
    QLabel* titleLabel_ = nullptr;
};

}  // namespace FreeWorksGui
