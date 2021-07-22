#include "rect.hpp"

GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::Rect)

glaxnimate::math::bezier::Bezier glaxnimate::model::Rect::to_bezier(model::FrameTime t) const
{
    math::bezier::Bezier bezier;
    QRectF bb = local_bounding_rect(t);
    float rounded = this->rounded.get_at(t);
    float max_r = std::min(bb.width()/2, bb.height()/2);
    if ( rounded > max_r )
        rounded = max_r;

    if ( rounded == 0 && !this->rounded.animated() )
    {
        bezier.add_point(bb.topLeft());
        bezier.add_point(bb.topRight());
        bezier.add_point(bb.bottomRight());
        bezier.add_point(bb.bottomLeft());
    }
    else
    {
        QPointF hh(rounded/2, 0);
        QPointF vh(0, rounded/2);
        QPointF hd(rounded, 0);
        QPointF vd(0, rounded);
        bezier.add_point(bb.topLeft()+vd, {0,0}, -vh);
        bezier.add_point(bb.topLeft()+hd, -hh);
        bezier.add_point(bb.topRight()-hd, {0,0}, hh);
        bezier.add_point(bb.topRight()+vd, -vh);
        bezier.add_point(bb.bottomRight()-vd, {0,0}, vh);
        bezier.add_point(bb.bottomRight()-hd, hh);
        bezier.add_point(bb.bottomLeft()+hd, {0,0}, -hh);
        bezier.add_point(bb.bottomLeft()-vd, vh);
    }

    bezier.close();
    return bezier;
}
