/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "visitor.hpp"

#include "model/document_node.hpp"
#include "model/document.hpp"
#include "model/assets/assets.hpp"
#include "model/assets/composition.hpp"


void glaxnimate::model::Visitor::visit(glaxnimate::model::Document* doc, model::Composition* main, bool skip_locked)
{
    on_visit(doc, main);
    visit(doc->assets(), skip_locked);
    on_visit_end(doc, main);
}

void glaxnimate::model::Visitor::visit(glaxnimate::model::DocumentNode* node, bool skip_locked)
{
    if ( skip_locked )
    {
        auto visual = node->cast<VisualNode>();
        if ( visual && visual->locked.get() )
            return;
    }

    on_visit(node);
    for ( auto ch : node->docnode_children() )
        visit(ch, skip_locked);
    on_visit_end(node);
}
