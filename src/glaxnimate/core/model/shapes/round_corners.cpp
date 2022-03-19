#include "round_corners.hpp"

GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::RoundCorners)

QIcon glaxnimate::model::RoundCorners::static_tree_icon()
{
    return QIcon::fromTheme("transform-affect-rounded-corners");
}

QString glaxnimate::model::RoundCorners::static_type_name_human()
{
    return tr("Round Corners");
}

bool glaxnimate::model::RoundCorners::process_collected() const
{
    return false;
}

static std::pair<QPointF, QPointF> get_vert_tan(const glaxnimate::math::bezier::Bezier& bezier, const QPointF& current_vertex, int closest_index, qreal round_distance)
{
    // value from https://spencermortensen.com/articles/bezier-circle/
    static const qreal tangent_length = 0.5519;
    if ( closest_index < 0 )
        closest_index += bezier.size();
    auto closest_vertex = bezier[closest_index].pos;
    auto distance = glaxnimate::math::length(current_vertex - closest_vertex);
    auto new_pos_perc = distance != 0 ? glaxnimate::math::min(distance/2, round_distance) / distance : 0;
    auto vertex = current_vertex + (closest_vertex - current_vertex) * new_pos_perc;
    auto tangent = - (vertex - current_vertex) * tangent_length;
    return {vertex, tangent};
}

static glaxnimate::math::bezier::Bezier round_corners(const glaxnimate::math::bezier::Bezier& original, qreal round_distance)
{
    glaxnimate::math::bezier::Bezier result;
    result.set_closed(original.closed());

    for ( int i = 0; i < original.size(); i++ )
    {
        if ( !original.closed() && (i == 0 || i == original.size() - 1) )
        {
            result.push_back(original[i]);
        }
        else
        {
            QPointF vert1, vert2, out_t, in_t;
            std::tie(vert1, out_t) = get_vert_tan(original, original[i].pos, i - 1, round_distance);
            result.add_point(vert1, {0, 0}, out_t);
            std::tie(vert2, in_t) = get_vert_tan(original, original[i].pos, i + 1, round_distance);
            result.add_point(vert2, in_t, {0, 0});
        }
    }

    return result;
}

glaxnimate::math::bezier::MultiBezier glaxnimate::model::RoundCorners::process(glaxnimate::model::FrameTime t, const math::bezier::MultiBezier& mbez) const
{
    qreal round_distance = radius.get_at(t);

    math::bezier::MultiBezier result;
    for ( const auto& inbez : mbez.beziers() )
        result.beziers().push_back(round_corners(inbez, round_distance));

    return result;
}
