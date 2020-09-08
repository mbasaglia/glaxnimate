#include <pybind11/operators.h>

#include "model/document.hpp"
#include "model/layers/layers.hpp"
#include "model/shapes/shapes.hpp"

#include "app/scripting/python/register_machinery.hpp"


template<class T>
void register_animatable(py::module& m)
{
    std::string name = "AnimatedProperty_";
    name += QMetaType::typeName(qMetaTypeId<T>());
    py::class_<model::AnimatedProperty<T>, model::AnimatableBase>(m, name.c_str());
}

PYBIND11_EMBEDDED_MODULE(glaxnimate, m)
{
    using namespace app::scripting::python;

    py::module utils = m.def_submodule("utils", "");
    py::class_<QColor>(utils, "Color")
        .def(py::init<int, int, int, int>())
        .def(py::init<>())
        .def(py::init<int, int, int>())
        .def(py::init<QString>())
        .def_property("red", &QColor::red, &QColor::setRed)
        .def_property("green", &QColor::red, &QColor::setRed)
        .def_property("blue", &QColor::blue, &QColor::setBlue)
        .def_property("name",
            [](const QColor& c){return c.name();},
            [](QColor& c, const QString& name){c.setNamedColor(name);}
        )
        .def("__str__", [](const QColor& c){return c.name();})
        .def("__repr__", qdebug_operator_to_string<QColor>())
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
        .def("__repr__", qdebug_operator_to_string<QSizeF>())
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

    py::module detail = m.def_submodule("__detail", "");
    py::class_<QObject>(detail, "__QObject");

    py::module model = m.def_submodule("model", "");
    py::class_<model::Object, QObject>(model, "Object");
    register_from_meta<model::Document, QObject>(model);
    register_from_meta<model::DocumentNode, model::Object>(model);
    register_from_meta<model::Composition, model::DocumentNode>(model);
    register_from_meta<model::MainComposition, model::Composition>(model);
    register_from_meta<model::AnimatableBase, QObject>(model);
    register_animatable<QPointF>(detail);
    register_animatable<QSizeF>(detail);
    register_animatable<QVector2D>(detail);
    register_animatable<QColor>(detail);
    register_animatable<float>(detail);

    py::module layers = model.def_submodule("layers", "");
    register_from_meta<model::Layer, model::DocumentNode>(layers);
    register_from_meta<model::ShapeLayer, model::Layer>(layers);
    register_from_meta<model::EmptyLayer, model::Layer>(layers);
    register_from_meta<model::SolidColorLayer, model::Layer>(layers);

    py::module shapes = model.def_submodule("shapes", "");
    register_from_meta<model::ShapeElement, model::DocumentNode>(shapes);
    register_from_meta<model::Shape, model::ShapeElement>(shapes);
    register_from_meta<model::Modifier, model::ShapeElement>(shapes);

    QMetaProperty prop;
    const QMetaObject& mo = model::Rect::staticMetaObject;
    for ( int i = 0; i < mo.propertyCount(); i++ )
        if ( mo.property(i).name() == QString("position") )
            prop = mo.property(i);
    register_from_meta<model::Rect, model::Shape>(shapes)
    .def("foo", [](model::Rect& r)->model::AnimatableBase*{
        return &r.position;
    }, py::return_value_policy::automatic_reference)
    .def("foo1", [prop](model::Rect& r)->model::AnimatableBase*{
        return prop.read(&r).value<model::AnimatableBase*>();
    }, py::return_value_policy::automatic_reference)
    .def("foo2", [prop](model::Rect& r)->QObject*{
        return prop.read(&r).value<QObject*>();
    }, py::return_value_policy::automatic_reference)
    .def("foo3", [](model::Rect& r)->QObject*{
        return &r.position;
    }, py::return_value_policy::automatic_reference)
    .def("bar", [](model::Rect& r)->model::Rect*{
        return &r;
    }, py::return_value_policy::automatic_reference)
    .def("bar1", [](model::Rect& r)->QObject*{
        return &r;
    }, py::return_value_policy::automatic_reference)
    ;
    register_from_meta<model::Ellipse, model::Shape>(shapes);
    register_from_meta<model::Group, model::Shape>(shapes);

    register_from_meta<model::BaseFill, model::Modifier>(shapes, enums<model::Fill::Rule>{});
    register_from_meta<model::Fill, model::BaseFill>(shapes);
    register_from_meta<model::BaseStroke, model::Modifier>(shapes, enums<model::Stroke::Cap, model::Stroke::Join>{});
    register_from_meta<model::Stroke, model::BaseStroke>(shapes);
}
