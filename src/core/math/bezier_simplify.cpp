#include "bezier_simplify.hpp"
#include <vector>


qreal triangle_area(const math::Bezier& curve, int point)
{
    QPointF prev = curve[point-1].pos;
    QPointF here = curve[point].pos;
    QPointF next = curve[point+1].pos;

    return qAbs(
        prev.x() * here.y() - here.x() * prev.y() +
        here.x() * next.y() - next.x() * here.y() +
        next.x() * prev.y() - prev.x() * next.y()
    );
}

void math::simplify(math::Bezier& curve, qreal threshold)
{
    if ( curve.size() < 3 || threshold <= 0 )
        return;

    // Algorithm based on https://bost.ocks.org/mike/simplify/
    std::vector<qreal> tris;

    tris.reserve(curve.size());

    tris.push_back(threshold); // [0] not used but keeping it for my own sanity
    for ( int i = 1; i < curve.size() - 1; i++ )
        tris.push_back(triangle_area(curve, i));

    while ( !tris.empty() )
    {
        qreal min = threshold;
        int index = -1;

        for ( int i = 0; i < int(tris.size()); i++ )
        {
            if ( tris[i] < min )
            {
                index = i;
                min = tris[i];
            }
        }
        if ( index == -1 )
            break;

        tris.erase(tris.begin() + index);
        curve.points().erase(curve.begin() + index);

        if ( index < int(tris.size()) )
            tris[index] = triangle_area(curve, index);
        if ( index > 1 )
            tris[index-1] = triangle_area(curve, index - 1);
    }

    // Fake smoothness by making line segments curved
    for ( int i = 1; i < curve.size() - 1; i++ )
    {
        curve[i].tan_in = math::lerp(curve[i].pos, curve[i-1].pos, 1/3.);
        curve[i].tan_out = math::lerp(curve[i].pos, curve[i+1].pos, 1/3.);
        curve[i].set_point_type(math::Smooth);
    }

}
