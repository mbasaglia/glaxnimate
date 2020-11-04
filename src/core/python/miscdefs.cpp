#include <pybind11/operators.h>

#include <QVector2D>
#include <QPointF>
#include <QSize>
#include <QSizeF>
#include <QRectF>
#include <QColor>
#include <QFile>
#include <QGuiApplication>

#include "app/log/log.hpp"

#include "math/bezier/bezier.hpp"

#include "miscdefs.hpp"

void define_bezier(py::module& m)
{
    py::module bezier = m.def_submodule("bezier", "");

    py::enum_<math::bezier::PointType>(bezier, "PointType")
        .value("Corner", math::bezier::Corner)
        .value("Smooth", math::bezier::Smooth)
        .value("Symmetrical", math::bezier::Symmetrical)
    ;

    py::class_<math::bezier::Point>(bezier, "Point")
        .def(py::init<QPointF>())
        .def(
            py::init<QPointF, QPointF, QPointF, math::bezier::PointType>(),
            py::arg("pos"), py::arg("tan_in"), py::arg("tan_out"), py::arg("type") = math::bezier::Corner
        )
        .def_readwrite("pos", &math::bezier::Point::pos)
        .def_property("tan_in", [](const math::bezier::Point& p){return p.tan_in;}, &math::bezier::Point::drag_tan_in)
        .def_property("tan_out", [](const math::bezier::Point& p){return p.tan_out;}, &math::bezier::Point::drag_tan_out)
        .def_property("type", [](const math::bezier::Point& p){return p.type;}, &math::bezier::Point::set_point_type)
        .def("translate", &math::bezier::Point::translate)
        .def("translate_to", &math::bezier::Point::translate_to)
    ;

    py::class_<math::bezier::Bezier>(bezier, "Bezier")
        .def(py::init<>())
        .def("__len__", &math::bezier::Bezier::size)
        .def_property_readonly("empty", &math::bezier::Bezier::empty)
        .def("clear", &math::bezier::Bezier::clear)
        .def("__getitem__", [](math::bezier::Bezier& b, int index){ return b[index];}, no_own)
        .def_property("closed", &math::bezier::Bezier::closed, &math::bezier::Bezier::set_closed)
        .def("insert_point", &math::bezier::Bezier::insert_point, no_own)
        .def("add_point", &math::bezier::Bezier::add_point, no_own)
        .def("add_smooth_point", &math::bezier::Bezier::add_smooth_point, no_own)
        .def("close", &math::bezier::Bezier::close, no_own)
        .def("line_to", &math::bezier::Bezier::line_to, no_own)
        .def("quadratic_to", &math::bezier::Bezier::quadratic_to, no_own)
        .def("cubic_to", &math::bezier::Bezier::cubic_to, no_own)
        .def("reverse", &math::bezier::Bezier::reverse)
        .def("bounding_box", &math::bezier::Bezier::bounding_box)
        .def("split_segment", &math::bezier::Bezier::split_segment)
        .def("split_segment_point", &math::bezier::Bezier::split_segment_point)
        .def("remove_point", &math::bezier::Bezier::remove_point)
        .def("lerp", &math::bezier::Bezier::lerp)
    ;

    pybind11::detail::type_caster<QVariant>::add_custom_type<math::bezier::Bezier>();
}

void define_utils(py::module& m)
{
    py::module utils = m.def_submodule("utils", "");
    py::class_<QColor>(utils, "Color")
        .def(py::init<int, int, int, int>())
        .def(py::init<>())
        .def(py::init<int, int, int>())
        .def(py::init<QString>())
        .def_property("red", &QColor::red, &QColor::setRed, "Red component between 0 and 255")
        .def_property("green", &QColor::red, &QColor::setRed, "Green component between 0 and 255")
        .def_property("blue", &QColor::blue, &QColor::setBlue, "Blue component between 0 and 255")
        .def_property("alpha", &QColor::alpha, &QColor::setAlpha, "Transparency component between 0 and 255")
        .def_property("name", qOverload<>(&QColor::name), qOverload<const QString&>(&QColor::setNamedColor))
        .def("__str__", [](const QColor& c){return c.name();})
        .def("__repr__", qdebug_operator_to_string<QColor>())
        .def(py::self == py::self)
        .def(py::self != py::self)
    ;
    py::class_<QPointF>(utils, "Point")
        .def(py::init<>())
        .def(py::init<double, double>())
        .def_property("x", &QPointF::x, &QPointF::setX)
        .def_property("y", &QPointF::y, &QPointF::setY)
        .def(py::self += py::self)
        .def(py::self + py::self)
        .def(py::self -= py::self)
        .def(py::self - py::self)
        .def(py::self * qreal())
        .def(py::self *= qreal())
        .def(py::self / qreal())
        .def(py::self /= qreal())
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def_property_readonly("length", &math::length<QPointF>)
        .def("__repr__", qdebug_operator_to_string<QPointF>())
    ;
    py::class_<QSizeF>(utils, "Size")
        .def(py::init<>())
        .def(py::init<double, double>())
        .def_property("width", &QSizeF::width, &QSizeF::setWidth)
        .def_property("height", &QSizeF::height, &QSizeF::setHeight)
        .def(py::self += py::self)
        .def(py::self + py::self)
        .def(py::self -= py::self)
        .def(py::self - py::self)
        .def(py::self * qreal())
        .def(py::self *= qreal())
        .def(py::self / qreal())
        .def(py::self /= qreal())
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def("to_point", [](const QSizeF& sz){ return QPointF(sz.width(), sz.height()); })
        .def("__repr__", qdebug_operator_to_string<QSizeF>())
    ;
    py::class_<QSize>(utils, "IntSize")
        .def(py::init<>())
        .def(py::init<int, int>())
        .def_property("width", &QSize::width, &QSize::setWidth)
        .def_property("height", &QSize::height, &QSize::setHeight)
        .def(py::self += py::self)
        .def(py::self + py::self)
        .def(py::self -= py::self)
        .def(py::self - py::self)
        .def(py::self * qreal())
        .def(py::self *= qreal())
        .def(py::self / qreal())
        .def(py::self /= qreal())
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def("to_point", [](const QSize& sz){ return QPointF(sz.width(), sz.height()); })
        .def("__repr__", qdebug_operator_to_string<QSize>())
    ;
    py::class_<QVector2D>(utils, "Vector2D")
        .def(py::init<>())
        .def(py::init<double, double>())
        .def_property("x", &QVector2D::x, &QVector2D::setX)
        .def_property("y", &QVector2D::y, &QVector2D::setY)
        .def(py::self += py::self)
        .def(py::self + py::self)
        .def(py::self -= py::self)
        .def(py::self - py::self)
        .def(py::self * qreal())
        .def(py::self *= qreal())
        .def(py::self / qreal())
        .def(py::self /= qreal())
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def("__getitem__", [](const QVector2D& v, int i) -> QVariant {
            if ( i >= 0 && i < 2 ) return v[i];
            return {};
        })
        .def("normalize", &QVector2D::normalize)
        .def("normalized", &QVector2D::normalized)
        .def("to_point", &QVector2D::toPointF)
        .def("length", &QVector2D::length)
        .def("length_squared", &QVector2D::lengthSquared)
        .def("__repr__", qdebug_operator_to_string<QVector2D>())
    ;
    py::class_<QRectF>(utils, "Rect")
        .def(py::init<>())
        .def(py::init<double, double, double, double>())
        .def(py::init<QPointF, QPointF>())
        .def(py::init<QPointF, QSizeF>())
        .def_property("left", &QRectF::left, &QRectF::setLeft)
        .def_property("right", &QRectF::right, &QRectF::setRight)
        .def_property("top", &QRectF::top, &QRectF::setTop)
        .def_property("bottom", &QRectF::bottom, &QRectF::setBottom)
        .def_property("center", &QRectF::center, &QRectF::moveCenter)
        .def_property("top_left", &QRectF::topLeft, &QRectF::setTopLeft)
        .def_property("top_right", &QRectF::topRight, &QRectF::setTopRight)
        .def_property("bottom_right", &QRectF::bottomRight, &QRectF::setBottomRight)
        .def_property("bottom_left", &QRectF::bottomLeft, &QRectF::setBottomLeft)
        .def_property("size", &QRectF::size, &QRectF::setSize)
    ;

    define_bezier(utils);
}

py::module define_detail(py::module& parent)
{
    py::module detail = parent.def_submodule("__detail", "");
    py::class_<QObject>(detail, "__QObject");
    py::class_<QIODevice>(detail, "QIODevice")
        .def("write", qOverload<const QByteArray&>(&QIODevice::write))
        .def("close", &QIODevice::close)
        .def_property_readonly("closed", [](const QIODevice& f){ return !f.isOpen(); })
        .def("readable", [](const QIODevice& f){ return f.openMode() & QIODevice::ReadOnly; })
        .def("seek", [](QIODevice& f, qint64 off, int whence){
            switch ( whence ) {
                case 0:
                    f.seek(off);
                    break;
                case 1:
                    f.seek(off + f.pos());
                    break;
                case 2:
                    f.readAll();
                    f.seek(off + f.pos());
                    break;
            }
        }, py::arg("offset"), py::arg("whence") = 0)
        .def("seekable", [](const QIODevice& f){ return !f.isSequential(); } )
        .def("tell", &QIODevice::pos)
        .def("writable", [](const QIODevice& f){ return f.openMode() & QIODevice::WriteOnly; })
        .def("read", [](QIODevice& f, qint64 size) {
            if ( size == qint64(-1) )
                return f.readAll();
            return f.read(size);
        }, py::arg("size") = qint64(-1))
        .def("readall", &QIODevice::readAll)
        .def("readline", [](QIODevice& f){ return f.readLine(); })
        .def("open", [](QIODevice&f, const QString& mode){
            QIODevice::OpenMode flags;
            if ( mode.contains('w') )
                flags |= QIODevice::WriteOnly;
            if ( mode.contains('r') )
                flags |= QIODevice::ReadOnly;
            if ( mode.contains('a') )
                flags |= QIODevice::Append|QIODevice::WriteOnly;
            if ( !mode.contains('b') )
                flags |= QIODevice::Text;
            f.open(flags);
        })
    ;
    py::class_<QFile, QIODevice>(detail, "QFile")
        .def("flush", &QFile::flush)
    ;

    py::class_<QGuiApplication>(detail, "QGuiApplication");

    return detail;
}

void define_log(py::module& m)
{
    py::module log = m.def_submodule("log", "Logging utilities");
    log.def("info", [](const QString& msg){ app::log::Log("Python").log(msg, app::log::Info); });
    log.def("warning", [](const QString& msg){ app::log::Log("Python").log(msg, app::log::Warning); });
    log.def("error", [](const QString& msg){ app::log::Log("Python").log(msg, app::log::Error); });
}


class HeadlessManager
{
public:
    void enter()
    {
        if ( !qGuiApp )
            app = std::make_unique<QGuiApplication>(fake_argc, (char**)fake_argv);
    }

    void exit(const pybind11::args&)
    {
        app.reset();
    }

private:
    std::unique_ptr<QGuiApplication> app;
    int fake_argc;
    const char* fake_argv = {""};
};

void define_environment(py::module& glaxnimate_module)
{
    py::module environment = glaxnimate_module.def_submodule("environment", "");

    py::class_<HeadlessManager>(environment, "Headless")
        .def(py::init<>())
        .def("__enter__", &HeadlessManager::enter)
        .def("__exit__", &HeadlessManager::exit)
        .attr("__doc__") = "Context manager that initializes a headless environment"
    ;
}
