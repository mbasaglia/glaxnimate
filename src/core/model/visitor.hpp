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
        visit(doc->main_composition());
    }

    void visit(model::DocumentNode* node)
    {
        on_visit(node);
        for ( auto ch : node->docnode_children() )
            visit(ch);
    }

private:
    virtual void on_visit(model::DocumentNode* node) = 0;
    virtual void on_visit(model::Document*) {}
};

} // namespace model
