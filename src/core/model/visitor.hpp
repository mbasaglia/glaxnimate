#pragma once

#include "model/document_node.hpp"
#include "model/document.hpp"

namespace model {


class Visitor
{
public:
    virtual ~Visitor() {}

    void visit(model::Document* doc)
    {
        on_visit(doc);
        visit(doc->main());
        on_visit_end(doc);
    }

    void visit(model::DocumentNode* node)
    {
        on_visit(node);
        for ( auto ch : node->docnode_children() )
            visit(ch);
        on_visit_end(node);
    }

private:
    virtual void on_visit(model::DocumentNode* node) = 0;
    virtual void on_visit_end(model::DocumentNode* node) { Q_UNUSED(node) }
    virtual void on_visit(model::Document* document) { Q_UNUSED(document) }
    virtual void on_visit_end(model::Document* document) { Q_UNUSED(document) }
};

} // namespace model