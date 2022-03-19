#include "inflate_deflate.hpp"

GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::InflateDeflate)

QIcon glaxnimate::model::InflateDeflate::static_tree_icon()
{
    return QIcon::fromTheme("zoom-draw");
}

QString glaxnimate::model::InflateDeflate::static_type_name_human()
{
    return tr("Inflate and Deflate");
}

bool glaxnimate::model::InflateDeflate::process_collected() const
{
    return false;
}

glaxnimate::math::bezier::MultiBezier glaxnimate::model::InflateDeflate::process(glaxnimate::model::FrameTime t, const math::bezier::MultiBezier& mbez) const
{
    if ( mbez.empty() )
        return {};

    auto amount = this->amount.get_at(t);

    if ( amount == 0 )
        return mbez;

    QPointF center;
    qreal count = 0;
    for ( const auto& bez : mbez.beziers() )
    {
        for ( const auto& point : bez )
        {
            center += point.pos;
        }
        count += bez.size();
    }
    if ( count == 0 )
        return mbez;

    center /= count;

    math::bezier::MultiBezier out;

    for ( const auto& in_bez : mbez.beziers() )
    {
        math::bezier::Bezier out_bez;
        for ( const auto& point : in_bez )
        {
            out_bez.points().push_back(math::bezier::Point(
                math::lerp(point.pos, center, amount),
                math::lerp(point.tan_in, center, -amount),
                math::lerp(point.tan_out, center, -amount)
            ));
        }
        if ( in_bez.closed() )
            out_bez.close();

        out.beziers().push_back(out_bez);
    }

    return out;
}
