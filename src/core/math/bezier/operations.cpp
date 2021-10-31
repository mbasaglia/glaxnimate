#include "operations.hpp"
#include <vector>

using namespace glaxnimate;


// Algorithm from https://www.particleincell.com/2012/bezier-splines/
void math::bezier::auto_smooth(math::bezier::Bezier& curve, int start, int end)
{
    if ( start < 0 || end > curve.size() || end - start < 2 )
        return;

    int n = end - start - 1;

    // rhs vector
    std::vector<qreal> a, b, c;
    std::vector<QPointF> r;

    // left most segment
    a.push_back(0);
    b.push_back(2);
    c.push_back(1);
    r.push_back(curve[start].pos + 2 * curve[start+1].pos);

    // internal segments
    for ( int i = 1; i < n - 1; i++ )
    {
        a.push_back(1);
        b.push_back(4);
        c.push_back(1);
        r.push_back(4 * curve[start+i].pos + 2 * curve[start+i+1].pos);
    }

    // right segment
    a.push_back(2);
    b.push_back(7);
    c.push_back(0);
    r.push_back(8 * curve[end-2].pos + curve[end-1].pos);

    // solves Ax=b with the Thomas algorithm (from Wikipedia)
    for ( int i = 1; i < n; i++ )
    {
        qreal m = a[i] / b[i-1];
        b[i] = b[i] - m * c[i - 1];
        r[i] = r[i] - m * r[i-1];
    }

    QPointF last = r[n-1]/b[n-1];
    curve[end-2].tan_in = last;
    for ( int i = n - 2; i >= 0; --i )
    {
        last = (r[i] - c[i] * last) / b[i];
        QPointF relative = (last - curve[start+i].pos);
        curve[start+i].tan_in = curve[start+i].pos - relative;
        curve[start+i].tan_out = curve[start+i].pos + relative;
        curve[start+i].type = math::bezier::Smooth;
    }

}


static qreal triangle_area(const math::bezier::Bezier& curve, int point)
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

void math::bezier::simplify(math::bezier::Bezier& curve, qreal threshold)
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
    auto_smooth(curve, 0, curve.size());

}

static math::bezier::ProjectResult project_extreme(int index, qreal t, const QPointF& p)
{
    return {index, t, math::length_squared(p), p};
}

static void project_impl(const math::bezier::CubicBezierSolver<QPointF>& solver, const QPointF& p, int index, math::bezier::ProjectResult& best)
{
    static constexpr const double min_dist = 0.01;

    math::bezier::ProjectResult left = project_extreme(index, 0, solver.points()[0]);
    math::bezier::ProjectResult right = project_extreme(index, 1, solver.points()[3]);
    math::bezier::ProjectResult middle = {index, 0, 0, {}};

    while ( true )
    {
        middle.factor = (left.factor + right.factor) / 2;
        middle.point = solver.solve(middle.factor);
        middle.distance = math::length_squared(middle.point);

        if ( right.distance < left.distance )
            left = middle;
        else
            right = middle;

        auto len = math::length_squared(left.point - right.point);
        if ( len <= min_dist || !std::isfinite(len) )
            break;
    }

    if ( right.distance < left.distance )
        left = right;

    if ( left.distance < best.distance )
    {
        best = left;
        best.point += p;
    }
}

static void project_impl(const math::bezier::Bezier& curve, const QPointF& p, int index, math::bezier::ProjectResult& best)
{
    math::bezier::CubicBezierSolver<QPointF> solver{
        curve[index].pos - p,
        curve[index].tan_out - p,
        curve[index + 1].tan_in - p,
        curve[index + 1].pos - p
    };

    project_impl(solver, p, index, best);
}

math::bezier::ProjectResult math::bezier::project(const math::bezier::Bezier& curve, const QPointF& p)
{
    if ( curve.empty() )
        return {0, 0, 0, p};

    if ( curve.size() == 1 )
        return {0, 0, math::length_squared(curve[0].pos - p), curve[0].pos};

    ProjectResult best {0, 0, std::numeric_limits<qreal>::max(), curve[0].pos};
    for ( int i = 0; i < curve.size() - 1; i++ )
        project_impl(curve, p, i, best);

    if ( curve.closed() )
        project_impl(curve, p, curve.size() - 1, best);

    return best;
}

math::bezier::ProjectResult math::bezier::project(const math::bezier::BezierSegment& segment, const QPointF& p)
{
    ProjectResult best {0, 0, std::numeric_limits<qreal>::max(), segment[0]};
    math::bezier::CubicBezierSolver<QPointF> solver{
        segment[0] - p,
        segment[1] - p,
        segment[2] - p,
        segment[3] - p
    };

    project_impl(solver, p, 0, best);

    return best;
}
