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

// Algoritm from https://www.particleincell.com/2012/bezier-splines/
void auto_smooth(math::Bezier& curve)
{
    int n = curve.size() - 1;

    // rhs vector
    std::vector<qreal> a, b, c;
    std::vector<QPointF> r;

    // left most segment
    a.push_back(0);
    b.push_back(2);
    c.push_back(1);
    r.push_back(curve[0].pos + 2 * curve[1].pos);

    // internal segments
    for ( int i = 1; i < n - 1; i++ )
    {
        a.push_back(1);
        b.push_back(4);
        c.push_back(1);
        r.push_back(4 * curve[i].pos + 2 * curve[i+1].pos);
    }

    // right segment
    a.push_back(2);
    b.push_back(7);
    c.push_back(0);
    r.push_back(8 * curve[n-1].pos + curve[n].pos);

    // solves Ax=b with the Thomas algorithm (from Wikipedia)
    for ( int i = 1; i < n; i++ )
    {
        qreal m = a[i] / b[i-1];
        b[i] = b[i] - m * c[i - 1];
        r[i] = r[i] - m * r[i-1];
    }

    QPointF last = r[n-1]/b[n-1];
    curve[n-1].tan_in = last;
    for ( int i = n - 2; i >= 0; --i )
    {
        last = (r[i] - c[i] * last) / b[i];
        QPointF relative = (last - curve[i].pos);
        curve[i].tan_in = curve[i].pos - relative;
        curve[i].tan_out = curve[i].pos + relative;
        curve[i].type = math::Smooth;
    }

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

    // Fake smoothness
    auto_smooth(curve);

}
