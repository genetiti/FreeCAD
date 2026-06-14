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

#include <QString>
#include <QStyledItemDelegate>

class QModelIndex;
class QStyleOptionViewItem;

namespace App
{
class DocumentObject;
}

namespace FreeWorksGui
{

/**
 * FeatureManager row delegate: a THIN QStyledItemDelegate over the hosted
 * FwFeatureTree (a Gui::TreeWidget) that does TWO display-only jobs and NOTHING
 * else (no model mutation, no property write, no inline stylesheet, no hex color):
 *
 *  1. Origin-plane DISPLAY-NAME remap (reference-CAD parity). The three stock origin
 *     planes carry an internal PlaneRoles name (App::Datums.h:204) on their generic
 *     App::PropertyString "Role": XY_Plane / XZ_Plane / YZ_Plane. The delegate remaps
 *     the rendered DISPLAY TEXT ONLY — XY_Plane -> "Front Plane", XZ_Plane ->
 *     "Top Plane", YZ_Plane -> "Right Plane" — leaving the underlying object's Label
 *     UNTOUCHED (display-only, D-08). The role string is resolved through the STOCK
 *     tree item-object data path (QModelIndex -> the inherited DocumentObjectItem ->
 *     its public object()/ViewProvider -> App::DocumentObject -> getPropertyByName
 *     ("Role")), NOT a fabricated model and NOT DocumentObjectItem internals. Every
 *     non-plane row (feature / sketch / axis / point) is a pure pass-through.
 *
 *  2. Below-tip greying paint hook. When a row is flagged below-tip (a Gui-only item
 *     flag the rollback bar will set in Plan 03-03) the text is painted with the
 *     palette's DISABLED text role (QPalette::Disabled, QPalette::Text) — palette /
 *     native QStyle only, NEVER a Visibility/property write, NEVER setStyleSheet, no
 *     hex (D-06, UI-SPEC § Color). This plan provides the palette-driven hook; the
 *     live below-tip set is Plan 03-03.
 *
 * Compiled directly into FreeCADGui and used in-process (mirrors FwFeatureTree /
 * FwRibbon), so no cross-library export macro is required.
 */
class FwFeatureTreeDelegate: public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit FwFeatureTreeDelegate(QObject* parent = nullptr);
    ~FwFeatureTreeDelegate() override;

    /// A Gui-only data role (not an App property) the rollback bar sets in Plan 03-03
    /// to mark a row as below the tip so this delegate greys it. Stored on the item
    /// via setData(kBelowTipRole, true); read here for the disabled-text paint hook.
    static constexpr int kBelowTipRole = Qt::UserRole + 4201;

    /// The three reference-CAD plane display names (display-only remap targets). These
    /// tr() strings carry no trademark token (leak-grep clean).
    static QString frontPlaneName();
    static QString topPlaneName();
    static QString rightPlaneName();

protected:
    /// Remap the DISPLAY TEXT of the three origin planes (display-only) before the base
    /// renders the row. Resolves the underlying object + its PlaneRoles "Role" string
    /// through the stock tree item-object path; falls back to the normal Label text for
    /// every non-plane row or any index whose object cannot be resolved (pass-through).
    void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override;

    /// Below-tip greying hook: when the row carries the kBelowTipRole flag, paint with
    /// the palette's QPalette::Disabled QPalette::Text role (no hex, no stylesheet).
    void paint(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

private:
    /// Resolve the App::DocumentObject behind @p index through the STOCK item-object
    /// path: the view's itemFromIndex() -> the inherited DocumentObjectItem ->
    /// object()/ViewProvider -> App::DocumentObject. nullptr for a non-object row.
    App::DocumentObject* objectOfIndex(const QModelIndex& index) const;

    /// If @p obj is one of the three origin planes (decided by its generic "Role"
    /// PropertyString matching a PlaneRoles name), return the reference-CAD display
    /// name; otherwise return a null QString (the caller keeps the stock Label text).
    QString remappedPlaneName(const App::DocumentObject* obj) const;
};

}  // namespace FreeWorksGui
