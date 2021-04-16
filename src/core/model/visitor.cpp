#include "visitor.hpp"

#include "model/document_node.hpp"
#include "model/document.hpp"
#include "model/assets/assets.hpp"
#include "model/assets/precomposition.hpp"


void model::Visitor::visit(model::Document* doc, bool skip_locked)
{
    on_visit(doc);
    visit(doc->assets(), skip_locked);
    visit(doc->main(), skip_locked);
    on_visit_end(doc);
}

void model::Visitor::visit(model::DocumentNode* node, bool skip_locked)
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
