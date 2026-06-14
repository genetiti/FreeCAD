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

#include <string>
#include <vector>

#include <Gui/Tree.h>

class QPainter;
class QStyleOptionViewItem;
class QModelIndex;
class QTreeWidgetItem;

namespace App
{
class DocumentObject;
}

namespace Gui
{
class Document;
}

namespace FreeWorksGui
{

/**
 * FeatureManager tree: a THIN subclass of Gui::TreeWidget that scopes the view to
 * the active Body and (in later plans) restyles it reference-CAD-style.
 *
 * This is the Wave-0 spike surface (D-03): it proves that FreeCAD's existing
 * Gui::TreeWidget can be (a) constructed/mounted, (b) scoped to the active Body's
 * subtree, and (c) restyled via thin additive seams WITHOUT editing Tree.cpp bodies
 * and WITHOUT touching the non-virtual DocumentItem/DocumentObjectItem item
 * construction (Pitfall 2 — wanting to is the FAIL signal).
 *
 * Scoping is RENDERED by row HIDING, not painting: scopeToActiveBody() descends the
 * REAL nested tree topology from invisibleRootItem() (per-document Gui::DocumentItem
 * -> its child Bodies, which are NOT top-level items, Tree.cpp:4562), resolves each
 * Gui::DocumentObjectItem's underlying App object via the public
 * DocumentObjectItem::object() accessor (Tree.h:495), keeps the ancestor DocumentItem
 * visible, and hides every NON-active Body subtree via QTreeWidgetItem::setHidden(true)
 * — the SAME public-item API the base TreeWidget itself uses (Tree.cpp:4574/6070). It
 * NEVER writes a Visibility/property value: scoping is a Gui-render, not a model
 * change (D-06).
 *
 * The active Body is identified purely by the "PartDesign::Body" type-name STRING
 * literal (obj->getTypeId().getName()), so FreeWorks needs NO PartDesign include and
 * NO PartDesign link (D-07; mirrors the Phase-2 FwRibbonContext kSketchVpTypeName
 * precedent). The Body's ordered features are read link-free via
 * getPropertyByName("Group") (App::PropertyLinkList).
 *
 * Compiled directly into FreeCADGui and used in-process, so no cross-library export
 * macro is required (mirrors FwWorkbench / FwRibbon).
 */
class FwFeatureTree: public Gui::TreeWidget
{
    Q_OBJECT

public:
    explicit FwFeatureTree(const char* name, QWidget* parent = nullptr);
    ~FwFeatureTree() override;

    /// The Body type-name contract literal — compared against getTypeId().getName().
    /// The ONLY identifier FreeWorks uses to recognise a Body (no PartDesign link).
    static constexpr const char* kBodyTypeName = "PartDesign::Body";

    /// Pure scope predicate: true iff @p obj is a PartDesign Body, decided purely by
    /// the "PartDesign::Body" type-name string (no module link). Kept pure so the
    /// decision is testable without a widget (decide-then-act, mirrors FwRibbonContext).
    bool isBody(const App::DocumentObject* obj) const;

    /// Record the Gui::Document the tree should scope (the stock TreeWidget already
    /// auto-populates DocumentItems from Gui signals; this just pins the scope target).
    void setDocument(Gui::Document* doc);

    /// Record the active Body the scope render should keep visible. Observed live via
    /// the Gui signals connected in the ctor; exposed for the headless QTEST.
    void setActiveBody(App::DocumentObject* body);

    /**
     * Scope the view to the active Body's subtree. Descends RECURSIVELY from
     * invisibleRootItem() through each per-document Gui::DocumentItem, resolves each
     * Gui::DocumentObjectItem's App object via the public object() accessor, keeps the
     * ancestor DocumentItem visible, hides every NON-active Body subtree via
     * setHidden(true), and un-hides the active Body's subtree. Never iterates
     * topLevelItem(i) to find Bodies (they are children of the DocumentItem), never
     * touches item construction, never writes a Visibility/property value.
     */
    void scopeToActiveBody();

    /// Read the active Body's ordered child features link-free via
    /// getPropertyByName("Group") (App::PropertyLinkList). Empty if absent.
    std::vector<App::DocumentObject*> activeBodyGroup() const;

protected:
    /// Presentation seam (Plan 03-03 paints the rollback band here). For the spike it
    /// is a pass-through that defers entirely to the base TreeWidget rendering.
    void drawRow(QPainter* painter,
                 const QStyleOptionViewItem& option,
                 const QModelIndex& index) const override;

private:
    /// Connect the Gui signals (activated object / in-edit / active document) so the
    /// scope render re-runs on active-Body change. Observe-the-DOM only, no App link.
    void connectActiveBodySignals();

    /// Recurse from @p item, hiding non-active-Body subtrees via setHidden. Returns
    /// nothing; the active Body subtree is left/forced visible.
    void scopeItemRecursive(QTreeWidgetItem* item);

    /// Resolve the App::DocumentObject behind a tree item via the public
    /// DocumentObjectItem::object() accessor; nullptr for a DocumentItem/non-object.
    App::DocumentObject* objectOfItem(QTreeWidgetItem* item) const;

    /// Install the spike presentation metrics (row height / indent) — palette/QStyle
    /// only, NO setStyleSheet, NO hex color (UI-SPEC § Color; Phase 7 owns theming).
    void applySpikeMetrics();

    Gui::Document* m_scopedDocument = nullptr;
    App::DocumentObject* m_activeBody = nullptr;
};

}  // namespace FreeWorksGui
