#pragma once

#include "visitor.hpp"

namespace glaxnimate::model {

template<class NodeType, class Callback>
class SimpleVisitor : public Visitor
{
public:
    SimpleVisitor(Callback callback) : callback(std::move(callback)) {}

private:
    void on_visit(model::DocumentNode* node) override
    {
        if ( auto obj = node->cast<NodeType>() )
            callback(obj);
    }

    Callback callback;
};

template<class NodeType, class Callback>
void simple_visit(model::DocumentNode* node, bool skip_locked, Callback callback)
{
    SimpleVisitor<NodeType, Callback>(std::move(callback)).visit(node, skip_locked);
}

} // namespace glaxnimate::model
