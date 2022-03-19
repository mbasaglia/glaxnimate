#include "visitor.hpp"

#include "glaxnimate/core/model/document_node.hpp"
#include "glaxnimate/core/model/document.hpp"
#include "glaxnimate/core/model/assets/assets.hpp"
#include "glaxnimate/core/model/assets/precomposition.hpp"


void glaxnimate::model::Visitor::visit(glaxnimate::model::Document* doc, bool skip_locked)
{
    on_visit(doc);
    visit(doc->assets(), skip_locked);
    visit(doc->main(), skip_locked);
    on_visit_end(doc);
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
