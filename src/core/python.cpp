#include "model/document.hpp"
#include "model/layers/layers.hpp"

#include "app/scripting/python/register_machinery.hpp"


PYBIND11_EMBEDDED_MODULE(glaxnimate, m)
{
    py::class_<QColor>(m, "QColor")
        .def(py::init<int, int, int, int>())
        .def(py::init<>())
        .def(py::init<int, int, int>())
        .def_property("red", &QColor::red, &QColor::setRed)
        .def_property("green", &QColor::red, &QColor::setRed)
        .def_property("blue", &QColor::blue, &QColor::setBlue)
    ;
    py::class_<QObject>(m, "__QObject");
    py::class_<model::Object, QObject>(m, "Object");
    using namespace app::scripting::python;
    register_from_meta<model::Document, QObject>(m);
    register_from_meta<model::DocumentNode, model::Object>(m);
    register_from_meta<model::Composition, model::DocumentNode>(m);
    register_from_meta<model::MainComposition, model::Composition>(m);
    register_from_meta<model::Layer, model::DocumentNode>(m);
    register_from_meta<model::ShapeLayer, model::Layer>(m);
    register_from_meta<model::EmptyLayer, model::Layer>(m);
}
