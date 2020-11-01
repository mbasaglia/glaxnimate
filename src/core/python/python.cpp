#include <pybind11/operators.h>

#include "app/log/log.hpp"

#include "model/document.hpp"
#include "model/shapes/shapes.hpp"
#include "model/defs/defs.hpp"
#include "model/defs/named_color.hpp"
#include "model/visitor.hpp"

#include "command/animation_commands.hpp"
#include "command/undo_macro_guard.hpp"
#include "command/object_list_commands.hpp"

#include "io/glaxnimate/glaxnimate_format.hpp"
#include "plugin/io.hpp"
#include "app_info.hpp"

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

    register_from_meta<io::ImportExport, QObject>(io)
        .def("progress_max_changed", &io::ImportExport::progress_max_changed)
        .def("progress", &io::ImportExport::progress)
    ;
    register_from_meta<io::glaxnimate::GlaxnimateFormat, io::ImportExport>(io)
        .attr("instance") = std::unique_ptr<io::glaxnimate::GlaxnimateFormat, py::nodelete>(io::glaxnimate::GlaxnimateFormat::instance())
    ;
    register_from_meta<plugin::IoFormat, io::ImportExport>(io);
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


template<class Owner, class PropT, class ItemT = typename PropT::value_type>
class CreateObject
{
public:
    using PtrMem = PropT Owner::*;

    CreateObject(PtrMem p) noexcept : ptr(p) {}

    ItemT* operator() (Owner* owner, const QString& clsname, int index = -1) const
    {
        return create(owner->document(), owner->*ptr, clsname, index);
    }


private:
    ItemT* create(model::Document* doc, PropT& prop, const QString& clsname, int index) const
    {
        auto obj = model::Factory::instance().build(clsname, doc);
        if ( !obj )
            return nullptr;

        auto cast = obj->cast<ItemT>();

        if ( !cast )
        {
            delete obj;
            return nullptr;
        }

        if constexpr ( std::is_base_of_v<model::DocumentNode, ItemT> )
            doc->set_best_name(static_cast<model::DocumentNode*>(cast));
        else
            cast->name.set(cast->type_name_human());
        doc->push_command(new command::AddObject<ItemT, PropT>(&prop, std::unique_ptr<ItemT>(cast), index));
        return cast;
    }

    PtrMem ptr;
};

template<class Owner, class PropT, class ItemT = typename PropT::value_type>
class CreateFixedObject
{
public:
    using PtrMem = PropT Owner::*;

    CreateFixedObject(PtrMem p) noexcept : ptr(p) {}

    ItemT* operator() (Owner* owner, int index = -1) const
    {
        return create(owner->document(), owner->*ptr, index);
    }


private:
    ItemT* create(model::Document* doc, PropT& prop, int index) const
    {
        auto ptr = new ItemT(doc);
        ptr->name.set(ptr->type_name_human());
        doc->push_command(new command::AddObject<ItemT, PropT>(&prop, std::unique_ptr<ItemT>(ptr), index));
        return ptr;
    }

    PtrMem ptr;
};

void register_py_module(py::module& glaxnimate_module)
{
    glaxnimate_module.attr("__version__") = AppInfo::instance().version();

    define_utils(glaxnimate_module);
    define_log(glaxnimate_module);

    py::module detail = glaxnimate_module.def_submodule("__detail", "");
    py::class_<QObject>(detail, "__QObject");
    py::class_<QIODevice>(detail, "QIODevice")
        .def("write", qOverload<const QByteArray&>(&QIODevice::write))
        .def("close", &QIODevice::close)
        .def_property_readonly("closed", [](const QIODevice& f){ return !f.isOpen(); })
        .def("readable", [](const QIODevice& f){ return f.openMode() & QIODevice::ReadOnly; })
        .def("seek", &QIODevice::seek)
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
    ;
    py::class_<QFile, QIODevice>(detail, "QFile");

    // for some reason some classes arent seen without this o_O
    static std::vector<int> foo = {
        qMetaTypeId<model::ReferenceTarget*>(),
        qMetaTypeId<model::NamedColor*>(),
        qMetaTypeId<model::Bitmap*>(),
    };

    define_io(glaxnimate_module);

    py::module model = glaxnimate_module.def_submodule("model", "");
    py::class_<model::Object, QObject>(model, "Object");

    py::class_<command::UndoMacroGuard>(model, "UndoMacroGuard")
        .def_readonly_static("__doc__", "Context manager that creates undo macros")
        .def("__enter__", &command::UndoMacroGuard::start)
        .def("__exit__", [](command::UndoMacroGuard& g, pybind11::object, pybind11::object, pybind11::object){
            g.finish();
        })
        .def("start", &command::UndoMacroGuard::start)
        .def("finish", &command::UndoMacroGuard::finish)

    ;

    register_from_meta<model::Document, QObject>(model)
        .def(py::init<QString>())
        .def(
            "macro",
             [](model::Document* document, const QString& str){
                return new command::UndoMacroGuard(str, document, false);
            },
            py::return_value_policy::take_ownership,
            "Context manager to group changes into a single undo command"
        );
    ;
    register_from_meta<model::ReferenceTarget, model::Object>(model);
    register_from_meta<model::DocumentNode, model::ReferenceTarget>(model);
    register_from_meta<model::AnimationContainer, model::Object>(model);
    register_from_meta<model::Transform, model::Object>(model);
    register_from_meta<model::Composition, model::DocumentNode>(model)
        .def("add_shape", CreateObject(&model::Composition::shapes), no_own,
            "Adds a shape from its class name",
             py::arg("type_name"),
             py::arg("index") = -1
        )
    ;
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
    register_from_meta<model::Defs, model::Object>(defs)
        .def("add_gradient", CreateFixedObject(&model::Defs::gradients), no_own, py::arg("index") = -1)
        .def("add_gradient_colors", CreateFixedObject(&model::Defs::gradient_colors), no_own, py::arg("index") = -1)
    ;


    py::module shapes = model.def_submodule("shapes", "");
    register_from_meta<model::ShapeElement, model::DocumentNode>(shapes);
    register_from_meta<model::Shape, model::ShapeElement>(shapes);
    register_from_meta<model::Modifier, model::ShapeElement>(shapes);
    register_from_meta<model::Styler, model::ShapeElement>(shapes);

    register_from_meta<model::Rect, model::Shape>(shapes);
    register_from_meta<model::Ellipse, model::Shape>(shapes);
    register_from_meta<model::PolyStar, model::Shape>(shapes);
    register_from_meta<model::Path, model::Shape>(shapes);

    register_from_meta<model::Group, model::ShapeElement>(shapes)
        .def("add_shape", CreateObject(&model::Group::shapes), no_own,
            "Adds a shape from its class name",
             py::arg("type_name"),
             py::arg("index") = -1
        )
    ;
    register_from_meta<model::Layer, model::Group>(shapes);

    register_from_meta<model::Fill, model::Styler>(shapes, enums<model::Fill::Rule>{});
    register_from_meta<model::Stroke, model::Styler>(shapes, enums<model::Stroke::Cap, model::Stroke::Join>{});

    register_from_meta<model::Image, model::ShapeElement>(shapes);
}
