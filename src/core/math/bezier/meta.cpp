#include "meta.hpp"

using namespace glaxnimate;

QDataStream& operator<<(QDataStream& ds, const math::bezier::Point& p)
{
    return ds << p.pos << p.tan_in << p.tan_out << qint16(p.type);
}

QDataStream& operator<<(QDataStream& ds, const math::bezier::Bezier& bez)
{
    ds << quint32(bez.size()) << bez.closed();
    for ( const auto& p : bez )
        ds << p;
    return ds;
}


QDataStream& operator>>(QDataStream& ds, math::bezier::Point& p)
{
    qint16 type = 0;
    ds >> p.pos >> p.tan_in >> p.tan_out >> type;
    p.type = math::bezier::PointType(type);
    return ds;
}

QDataStream& operator>>(QDataStream& ds, math::bezier::Bezier& bez)
{
    bez.clear();
    quint32 size = 0;
    bool closed = false;
    ds >> size >> closed;
    bez.set_closed(closed);
    for ( quint32 i = 0; i < size; i++ )
    {
        math::bezier::Point p{{}, {}, {}};
        ds >> p;
        bez.push_back(p);
    }
    return ds;
}

void math::bezier::register_meta()
{
    qRegisterMetaType<Bezier>("glaxnimate::math::bezier::Bezier");
    qRegisterMetaType<Point>("glaxnimate::math::bezier::Point");
#if QT_VERSION_MAJOR < 6
    qRegisterMetaTypeStreamOperators<Bezier>("glaxnimate::math::bezier::Bezier");
    qRegisterMetaTypeStreamOperators<Point>("glaxnimate::math::bezier::Point");
#endif
    QMetaType::registerConverter<Point, QPointF>(&Point::position);
    QMetaType::registerConverter<QPointF, Point>([](const QPointF& p) { return Point{p, p, p}; });
}


namespace {

class BezierAutoRegister
{
public:
    BezierAutoRegister()
    {
        math::bezier::register_meta();
    }
};

} // namespace

static BezierAutoRegister bezier_reg = {};
