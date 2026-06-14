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

// Name the Qt headers this TU uses unconditionally rather than relying on the PCH
// (mirrors FwRibbon.cpp / FwFeatureTree.cpp WR-04).
#include <QModelIndex>
#include <QPainter>
#include <QPalette>
#include <QStyleOptionViewItem>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <App/DocumentObject.h>
#include <App/PropertyStandard.h>

#include <Gui/Tree.h>
#include <Gui/ViewProviderDocumentObject.h>

#include "FwFeatureTreeDelegate.h"

namespace FreeWorksGui
{

namespace
{
// The generic App::PropertyString that carries a DatumElement's internal role name
// (App::Datums.h:47 `PropertyString Role`; set to a PlaneRoles value at
// Datums.cpp:292). Read through the generic getPropertyByName accessor so no Datums /
// PartDesign header or link is needed.
constexpr const char* kRolePropertyName = "Role";

// The three internal PlaneRoles names (App::Datums.h:204) the remap keys on. Kept as
// local literals so the role match reads the exact strings here; no Datums link.
constexpr const char* kRoleXyPlane = "XY_Plane";
constexpr const char* kRoleXzPlane = "XZ_Plane";
constexpr const char* kRoleYzPlane = "YZ_Plane";
}  // namespace

FwFeatureTreeDelegate::FwFeatureTreeDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{}

FwFeatureTreeDelegate::~FwFeatureTreeDelegate() = default;

QString FwFeatureTreeDelegate::frontPlaneName()
{
    return tr("Front Plane");
}

QString FwFeatureTreeDelegate::topPlaneName()
{
    return tr("Top Plane");
}

QString FwFeatureTreeDelegate::rightPlaneName()
{
    return tr("Right Plane");
}

App::DocumentObject* FwFeatureTreeDelegate::objectOfIndex(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return nullptr;
    }
    // Resolve through the STOCK item-object data path. The delegate's parent is the
    // hosting tree (a Gui::TreeWidget, which IS-A QTreeWidget); itemFromIndex() returns
    // the inherited DocumentObjectItem whose PUBLIC object() accessor (Tree.h:495)
    // exposes the ViewProvider -> App::DocumentObject. We never reach into
    // DocumentObjectItem internals and never fabricate a model.
    const auto* view = qobject_cast<const QTreeWidget*>(parent());
    if (view == nullptr) {
        return nullptr;
    }
    QTreeWidgetItem* item = view->itemFromIndex(index);
    auto* objItem = dynamic_cast<Gui::DocumentObjectItem*>(item);
    if (objItem == nullptr) {
        return nullptr;
    }
    Gui::ViewProviderDocumentObject* vp = objItem->object();
    return vp != nullptr ? vp->getObject() : nullptr;
}

QString FwFeatureTreeDelegate::remappedPlaneName(const App::DocumentObject* obj) const
{
    if (obj == nullptr) {
        return {};
    }
    // Read the internal role name from the generic "Role" PropertyString. A non-datum
    // row has no such property (nullptr) -> pass-through (null QString).
    const App::Property* prop = obj->getPropertyByName(kRolePropertyName);
    const auto* roleProp = dynamic_cast<const App::PropertyString*>(prop);
    if (roleProp == nullptr) {
        return {};
    }
    const std::string role = roleProp->getValue();
    if (role == kRoleXyPlane) {
        return frontPlaneName();
    }
    if (role == kRoleXzPlane) {
        return topPlaneName();
    }
    if (role == kRoleYzPlane) {
        return rightPlaneName();
    }
    // An axis / point / non-plane datum: keep the stock text (pass-through).
    return {};
}

void FwFeatureTreeDelegate::initStyleOption(QStyleOptionViewItem* option,
                                            const QModelIndex& index) const
{
    // Let the base populate the option from the model first (icon, default text,
    // palette, font) — we only override the rendered display text for the three
    // origin planes, and only the DISPLAY TEXT (the object Label is never written).
    QStyledItemDelegate::initStyleOption(option, index);

    const QString remapped = remappedPlaneName(objectOfIndex(index));
    if (!remapped.isEmpty()) {
        option->text = remapped;
    }
}

void FwFeatureTreeDelegate::paint(QPainter* painter,
                                  const QStyleOptionViewItem& option,
                                  const QModelIndex& index) const
{
    // Below-tip greying hook (palette-driven; the live below-tip set is Plan 03-03).
    // When the row carries the Gui-only kBelowTipRole flag, paint the text with the
    // palette's DISABLED text role — native QStyle / QPalette only, no hex, no inline
    // stylesheet, and NEVER a property write for presentation (D-06, UI-SPEC § Color).
    const QVariant belowTip = index.data(kBelowTipRole);
    if (belowTip.isValid() && belowTip.toBool()) {
        QStyleOptionViewItem greyed(option);
        const QColor disabledText = greyed.palette.color(QPalette::Disabled, QPalette::Text);
        greyed.palette.setColor(QPalette::Text, disabledText);
        greyed.palette.setColor(QPalette::HighlightedText, disabledText);
        QStyledItemDelegate::paint(painter, greyed, index);
        return;
    }
    QStyledItemDelegate::paint(painter, option, index);
}

}  // namespace FreeWorksGui
