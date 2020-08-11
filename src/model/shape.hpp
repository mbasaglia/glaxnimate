#pragma once

#include "document_node.hpp"

namespace model {
    
class ShapeElement : public DocumentNode
{
    Q_OBJECT
};


class Shape : public ShapeElement
{
    Q_OBJECT
};
    
} // namespace model
