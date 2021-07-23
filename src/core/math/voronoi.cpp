#include "voronoi.hpp"

#include <queue>
#include<memory>

namespace {

struct VoronoiSolver
{
    VoronoiSolver(std::vector<QPointF>&& points, const QRectF& bounds )
        : points(std::move(points)),
          bounds(bounds)
    {
    }

    void solve()
    {
        if ( points.empty() )
            return;

        arcs.push_back(std::make_unique<Arc>(points.back()));
        points.pop_back();

        while ( !points.empty() )
        {
            if ( !events.empty() && events.top()->x <= points.back().x)
                process_event();
            else
                process_point();
        }
    }

    struct Arc;

    struct Event
    {
        double x;
        QPointF point;
        Arc* arc;
        bool valid = true;

        Event(double x, const QPointF& point, Arc* arc)
            : x(x), point(point), arc(arc)
        {}
    };

    struct Segment
    {
        QPointF start;
        QPointF end;
        bool done = false;

        Segment(const QPointF& p) : start(p) {}
    };

    struct Arc
    {
        QPointF point;
        Event* event = nullptr;

        Segment* s0 = nullptr;
        Segment* s1 = nullptr;

        Arc(const QPointF& point)
            : point(point)
        {}
    };

    static bool event_cmp(const std::unique_ptr<Event>& a, const std::unique_ptr<Event>& b)
    {
        return a->x > b->x;
    }

    void process_point()
    {
        auto p = points.back();
        points.pop_back();
        front_insert(p);
    }

    void front_insert(const QPointF& p)
    {
        for ( std::size_t i = 0; i < arcs.size(); i++ )
        {
            QPointF z;
            auto cur = arcs[i].get();

            if ( intersect(p, i, &z) )
            {

                // New parabola intersects arc i.  If necessary, duplicate i.
                if ( i + 1 < arcs.size() )
                    intersect(p, i + 1, nullptr);

                arcs.insert(arcs.begin() + i + 1, std::make_unique<Arc>(cur->point));

                auto next = arcs[i+1].get();

                next->s1 = cur->s1;

                // Add p between i and i->next.
                arcs.insert(arcs.begin() + i + 1, std::make_unique<Arc>(p));
                i++; // Now i points to the new arc.

                auto prev = cur;
                cur = arcs[i].get();
                next = arcs[i+1].get();

                // Add new half-edges connected to i's endpoints.
                segments.push_back(std::make_unique<Segment>(z));
                prev->s1 = cur->s0 = segments.back().get();
                segments.push_back(std::make_unique<Segment>(z));
                next->s0 = cur->s1 = segments.back().get();

                // Check for new circle events around the new arc:
                check_circle_event(cur, p.x());
                check_circle_event(prev, p.x());
                check_circle_event(next, p.x());

                return;
            }
        }
    }

    bool intersect(const QPointF& p, int i, QPoint& *res)
    {
        auto cur = arcs[i].get();

        if ( cur->point.x() == p.x() )
            return false;

        double a = std::numeric_limits<double>::max();
        if ( i > 0 ) // Get the intersection of i->prev, i.
            a = intersection(arcs[i-1]->point, cur->point, p.x()).y();

        double b = std::numeric_limits<double>::max();
        if ( i + 1 < arcs.size() ) // Get the intersection of i->next, i.
            b = intersection(cur->point, arcs[i+1]->point, p.x()).y();

        if ( (!i->prev || a <= p.y) && (!i->next || p.y <= b)) {
            res->y = p.y;

            // Plug it back into the parabola equation.
            res->x = (i->p.x*i->p.x + (i->p.y-res->y)*(i->p.y-res->y) - p.x*p.x)
                        / (2*i->p.x - 2*p.x);

            return true;
        }
        return false;
    }


    std::vector<QPointF> points;
    std::vector<std::unique_ptr<Arc>> arcs;
    QRectF bounds;
    std::priority_queue<std::unique_ptr<Event>, std::vector<std::unique_ptr<Event>>, decltype(&VoronoiSolver::event_cmp)> events;
    std::vector<std::unique_ptr<Segment>> segments;
};

} // namespace


glaxnimate::math::bezier::MultiBezier glaxnimate::math::voronoi ( std::vector<QPointF> points, const QRectF& bounds )
{
    // smallest x at the end
    std::sort(points.begin(), points.end(), [](const QPointF& a, const QPointF& b) { return a.x() == b.x() ? a.y() > b.y() : a.x() > b.x();});

    VoronoiSolver solver(std::move(points), bounds);
    solver.solve();
}


