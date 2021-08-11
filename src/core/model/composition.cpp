#include "composition.hpp"
#include "document.hpp"


int glaxnimate::model::Composition::docnode_child_index(glaxnimate::model::DocumentNode* dn) const
{
    return shapes.index_of(static_cast<ShapeElement*>(dn));
}


QRectF glaxnimate::model::Composition::local_bounding_rect(FrameTime t) const
{
    if ( shapes.empty() )
        return QRectF(QPointF(0, 0), document()->size());
    return shapes.bounding_rect(t);
}

