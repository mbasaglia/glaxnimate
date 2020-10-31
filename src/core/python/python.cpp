#include <pybind11/operators.h>

#include "app/log/log.hpp"

#include "model/document.hpp"
#include "model/shapes/shapes.hpp"
#include "command/animation_commands.hpp"
#include "io/glaxnimate/glaxnimate_format.hpp"
#include "model/defs/defs.hpp"
#include "model/defs/named_color.hpp"
#include "model/visitor.hpp"
#include "command/undo_macro_guard.hpp"

#include "app/scripting/python/register_machinery.hpp"

auto no_own = py::return_value_policy::automatic_reference;
using namespace app::scripting::python;

template<class T>
void register_animatable(py::module& m)
{
    std::string name = "AnimatedProperty_";
    name += QMetaType::typeName(qMetaTypeId<T>());
    py::class_<model::AnimatedProperty<T>, model::AnimatableBase>(m, name.c_str());
}

void define_utils(py::module& m)
{
    py::module utils = m.def_submodule("utils", "");
    py::class_<QColor>(utils, "Color")
        .def(py::init<int, int, int, int>())
        .def(py::init<>())
        .def(py::init<int, int, int>())
        .def(py::init<QString>())
        .def_property("red", &QColor::red, &QColor::setRed)
        .def_property("green", &QColor::red, &QColor::setRed)
        .def_property("blue", &QColor::blue, &QColor::setBlue)
        .def_property("alpha", &QColor::alpha, &QColor::setAlpha)
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
        .def("__repr__", qdebug_operator_to_string<QSizeF>())
    ;
    py::class_<QSize>(utils, "IntSize")
        .def(py::init<>())
        .def(py::init<int, int>())
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
}


void define_io(py::module& m)
{
    py::module io = m.def_submodule("io", "Input/Output utilities");

    py::class_<io::mime::MimeSerializer>(io, "MimeSerializer")
        .def_property_readonly("slug", &io::mime::MimeSerializer::slug)
        .def_property_readonly("name", &io::mime::MimeSerializer::name)
        .def_property_readonly("mime_types", &io::mime::MimeSerializer::mime_types)
        .def("serialize", &io::mime::MimeSerializer::serialize)
    ;

    using Fac = io::IoRegistry;
    py::class_<Fac, std::unique_ptr<Fac, py::nodelete>>(io, "IoRegistry")
        .def("importers", &Fac::importers, no_own)
        .def("exporters", &Fac::exporters, no_own)
        .def("from_extension", &Fac::from_extension, no_own)
        .def("from_filename", &Fac::from_filename, no_own)
        .def("serializers", &Fac::serializers, no_own)
        .def("serializer_from_slug", &Fac::serializer_from_slug, no_own)
    ;

    io.attr("registry") = std::unique_ptr<Fac, py::nodelete>(&io::IoRegistry::instance());

    register_from_meta<io::ImportExport, QObject>(io);
    register_from_meta<io::glaxnimate::GlaxnimateFormat, io::ImportExport>(io)
        .attr("instance") = std::unique_ptr<io::glaxnimate::GlaxnimateFormat, py::nodelete>(io::glaxnimate::GlaxnimateFormat::instance())
    ;
}


void define_animatable(py::module& m)
{
    register_from_meta<model::KeyframeTransition, QObject>(m);
    py::class_<model::KeyframeBase>(m, "Keyframe")
        .def_property_readonly("time", &model::KeyframeBase::time)
        .def_property_readonly("value", &model::KeyframeBase::value)
        .def_property_readonly("transition",
            (const model::KeyframeTransition& (model::KeyframeBase::*)()const)
            &model::KeyframeBase::transition,
            no_own
        )
    ;
    register_from_meta<model::AnimatableBase, QObject>(m)
        .def("keyframe", [](const model::AnimatableBase& a, model::FrameTime t){ return a.keyframe(t); }, no_own)
        .def("set_keyframe", [](model::AnimatableBase& a, model::FrameTime time, const QVariant& value){
            a.object()->document()->undo_stack().push(
                new command::SetKeyframe(&a, time, value, true)
            );
            return a.keyframe(a.keyframe_index(time));
        }, no_own)
        .def("remove_keyframe_at_time", [](model::AnimatableBase& a, model::FrameTime time){
            a.object()->document()->undo_stack().push(
                new command::RemoveKeyframeTime(&a, time)
            );
        })
    ;
}


void define_log(py::module& m)
{
    py::module log = m.def_submodule("log", "Logging utilities");
    log.def("info", [](const QString& msg){ app::log::Log("Python").log(msg, app::log::Info); });
    log.def("warning", [](const QString& msg){ app::log::Log("Python").log(msg, app::log::Warning); });
    log.def("error", [](const QString& msg){ app::log::Log("Python").log(msg, app::log::Error); });
}

class PyVisitorPublic : public model::Visitor
{
public:
    virtual void on_visit_document(model::Document *){}
    virtual void on_visit_node(model::DocumentNode*){}

private:
    void on_visit(model::Document * document) override
    {
        on_visit_document(document);
    }

    void on_visit(model::DocumentNode * node) override
    {
        on_visit_node(node);
    }
};

class PyVisitorTrampoline : public PyVisitorPublic
{
public:
    void on_visit_document(model::Document * document) override
    {
        PYBIND11_OVERLOAD(void, PyVisitorPublic, on_visit_document, document);
    }

    void on_visit(model::DocumentNode * node) override
    {
        PYBIND11_OVERLOAD_PURE(void, PyVisitorPublic, on_visit_node, node);
    }
};

void register_py_module(py::module& glaxnimate_module)
{
    define_utils(glaxnimate_module);
    define_log(glaxnimate_module);

    py::module detail = glaxnimate_module.def_submodule("__detail", "");
    py::class_<QObject>(detail, "__QObject");
    py::class_<command::UndoMacroGuard>(detail, "UndoMacroGuard")
        .def("__enter__", &command::UndoMacroGuard::start)
        .def("__exit__", [](command::UndoMacroGuard& g, pybind11::object, pybind11::object, pybind11::object){
            g.finish();
        })
        .def("start", &command::UndoMacroGuard::start)
        .def("finish", &command::UndoMacroGuard::finish)
    ;

    // for some reason some classes arent seen without this o_O
    static std::vector<int> foo = {
        qMetaTypeId<model::ReferenceTarget*>(),
        qMetaTypeId<model::NamedColor*>(),
        qMetaTypeId<model::Bitmap*>(),
    };

    define_io(glaxnimate_module);

    py::module model = glaxnimate_module.def_submodule("model", "");
    py::class_<model::Object, QObject>(model, "Object");

    register_from_meta<model::Document, QObject>(model)
        .def("macro", [](model::Document* document, const QString& str){
            return new command::UndoMacroGuard(str, document, false);
        }, py::return_value_policy::take_ownership);
    ;
    register_from_meta<model::ReferenceTarget, model::Object>(model);
    register_from_meta<model::DocumentNode, model::ReferenceTarget>(model);
    register_from_meta<model::AnimationContainer, model::Object>(model);
    register_from_meta<model::Transform, model::Object>(model);
    register_from_meta<model::Composition, model::DocumentNode>(model);
    register_from_meta<model::MainComposition, model::Composition>(model);
    define_animatable(model);
    register_animatable<QPointF>(detail);
    register_animatable<QSizeF>(detail);
    register_animatable<QVector2D>(detail);
    register_animatable<QColor>(detail);
    register_animatable<float>(detail);
    register_animatable<QGradientStops>(detail);
    py::class_<PyVisitorPublic, PyVisitorTrampoline>(model, "Visitor")
        .def(py::init())
        .def("visit", (void (PyVisitorPublic::*)(model::Document*, bool))&PyVisitorPublic::visit)
        .def("visit", (void (PyVisitorPublic::*)(model::DocumentNode*, bool))&PyVisitorPublic::visit)
        .def("on_visit_document", &PyVisitorPublic::on_visit_document)
        .def("on_visit_node", &PyVisitorPublic::on_visit_node)
    ;

    py::module defs = model.def_submodule("defs", "");
    register_from_meta<model::Asset, model::ReferenceTarget>(defs);
    register_from_meta<model::BrushStyle, model::Asset>(defs);
    register_from_meta<model::NamedColor, model::BrushStyle>(defs);
    register_from_meta<model::GradientColors, model::Asset>(defs);
    register_from_meta<model::Gradient, model::BrushStyle>(defs);
    register_from_meta<model::Bitmap, model::Asset>(defs);
    register_from_meta<model::Defs, model::Object>(defs);


    py::module shapes = model.def_submodule("shapes", "");
    register_from_meta<model::ShapeElement, model::DocumentNode>(shapes);
    register_from_meta<model::Shape, model::ShapeElement>(shapes);
    register_from_meta<model::Modifier, model::ShapeElement>(shapes);
    register_from_meta<model::Styler, model::ShapeElement>(shapes);

    register_from_meta<model::Rect, model::Shape>(shapes);
    register_from_meta<model::Ellipse, model::Shape>(shapes);
    register_from_meta<model::PolyStar, model::Shape>(shapes);

    register_from_meta<model::Group, model::ShapeElement>(shapes);
    register_from_meta<model::Layer, model::Group>(shapes);

    register_from_meta<model::Fill, model::Styler>(shapes, enums<model::Fill::Rule>{});
    register_from_meta<model::Stroke, model::Styler>(shapes, enums<model::Stroke::Cap, model::Stroke::Join>{});

    register_from_meta<model::Image, model::ShapeElement>(shapes);
}
