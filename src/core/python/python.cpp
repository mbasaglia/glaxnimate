#include <pybind11/operators.h>

#include "model/document.hpp"
#include "model/visitor.hpp"

#include "model/shapes/group.hpp"
#include "model/shapes/layer.hpp"
#include "model/shapes/precomp_layer.hpp"
#include "model/shapes/rect.hpp"
#include "model/shapes/ellipse.hpp"
#include "model/shapes/path.hpp"
#include "model/shapes/polystar.hpp"
#include "model/shapes/fill.hpp"
#include "model/shapes/stroke.hpp"
#include "model/shapes/image.hpp"
#include "model/shapes/repeater.hpp"
#include "model/shapes/trim.hpp"

#include "model/assets/assets.hpp"
#include "model/assets/named_color.hpp"
#include "model/assets/precomposition.hpp"

#include "command/animation_commands.hpp"
#include "command/undo_macro_guard.hpp"
#include "command/object_list_commands.hpp"

#include "io/glaxnimate/glaxnimate_format.hpp"
#include "io/raster/raster_format.hpp"
#include "io/raster/raster_mime.hpp"
#include "io/svg/svg_format.hpp"
#include "io/svg/svg_renderer.hpp"


#include "plugin/io.hpp"
#include "app_info.hpp"

#include "miscdefs.hpp"

using namespace app::scripting::python;

template<class T, class Base=model::AnimatableBase>
void register_animatable(py::module& m)
{
    std::string name = "AnimatedProperty<";
    name += QMetaType::typeName(qMetaTypeId<T>());
    name += ">";
    py::class_<model::AnimatedProperty<T>, Base>(m, name.c_str());
}

static QImage doc_to_image(model::Document* doc)
{
    return io::raster::RasterMime::to_image({doc->main()});
}

static QByteArray frame_to_svg(model::Document* doc)
{
    QByteArray data;
    QBuffer file(&data);
    file.open(QIODevice::WriteOnly);

    io::svg::SvgRenderer rend(io::svg::NotAnimated);
    rend.write_document(doc);
    rend.write(&file, true);

    return data;
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

    const char* to_image_docstring = "Renders the current frame to an image";
    py::class_<io::raster::RasterMime, io::mime::MimeSerializer>(io, "RasterMime")
        .def_static("render_frame", &io::raster::RasterMime::to_image, to_image_docstring)
        .def_static("render_frame", &doc_to_image, to_image_docstring)
    ;

    using Fac = io::IoRegistry;
    py::class_<Fac, std::unique_ptr<Fac, py::nodelete>>(io, "IoRegistry")
        .def("importers", &Fac::importers, no_own)
        .def("exporters", &Fac::exporters, no_own)
        .def("from_extension", &Fac::from_extension, no_own)
        .def("from_filename", &Fac::from_filename, no_own)
        .def("from_slug", &Fac::from_slug, no_own)
        .def("__getitem__", &Fac::from_slug, no_own)
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

    register_from_meta<io::raster::RasterFormat, io::ImportExport>(io)
        .def_static("render_frame", &io::raster::RasterMime::to_image, to_image_docstring)
        .def_static("render_frame", &doc_to_image, to_image_docstring)
    ;

    register_from_meta<io::svg::SvgFormat, io::ImportExport>(io)
        .def_static("render_frame", &frame_to_svg, "renders the current frame to SVG")
    ;


    register_from_meta<plugin::IoFormat, io::ImportExport>(io);
}


void define_animatable(py::module& m)
{
    py::class_<model::KeyframeTransition> kt(m, "KeyframeTransition");
    kt.attr("Descriptive") = py::enum_<model::KeyframeTransition::Descriptive>(kt, "Descriptive")
        .value("Hold", model::KeyframeTransition::Hold)
        .value("Linear", model::KeyframeTransition::Linear)
        .value("Ease", model::KeyframeTransition::Ease)
        .value("Custom", model::KeyframeTransition::Custom)
    ;
    kt
        .def(py::init<>())
        .def(py::init<const QPointF&, const QPointF&>())
        .def_property("hold", &model::KeyframeTransition::hold, &model::KeyframeTransition::set_hold)
        .def_property("before", &model::KeyframeTransition::before, &model::KeyframeTransition::set_before)
        .def_property("after", &model::KeyframeTransition::after, &model::KeyframeTransition::set_after)
        .def_property("before_descriptive", &model::KeyframeTransition::before_descriptive, &model::KeyframeTransition::set_before_descriptive)
        .def_property("after_descriptive", &model::KeyframeTransition::after_descriptive, &model::KeyframeTransition::set_after_descriptive)
        .def("lerp_factor", &model::KeyframeTransition::lerp_factor)
        .def("bezier_parameter", &model::KeyframeTransition::bezier_parameter)
    ;

    py::class_<model::KeyframeBase>(m, "Keyframe")
        .def_property_readonly("time", &model::KeyframeBase::time)
        .def_property_readonly("value", &model::KeyframeBase::value)
        .def_property("transition",
            &model::KeyframeBase::transition,
            &model::KeyframeBase::set_transition,
            no_own
        )
    ;
    register_from_meta<model::AnimatableBase, QObject>(m)
        .def("keyframe", [](const model::AnimatableBase& a, model::FrameTime t){ return a.keyframe(t); }, no_own, py::arg("time"))
        .def("set_keyframe", [](model::AnimatableBase& a, model::FrameTime time, const QVariant& value){
            a.object()->document()->undo_stack().push(
                new command::SetKeyframe(&a, time, value, true)
            );
            return a.keyframe(a.keyframe_index(time));
        }, no_own, py::arg("time"), py::arg("value"))
        .def("remove_keyframe_at_time", [](model::AnimatableBase& a, model::FrameTime time){
            a.object()->document()->undo_stack().push(
                new command::RemoveKeyframeTime(&a, time)
            );
        }, py::arg("time"))
    ;
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
        auto obj = model::Factory::static_build(clsname, doc);
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


void register_py_module(py::module& glaxnimate_module)
{
    glaxnimate_module.attr("__version__") = AppInfo::instance().version();

    define_utils(glaxnimate_module);
    define_log(glaxnimate_module);
    py::module detail = define_detail(glaxnimate_module);
    define_environment(glaxnimate_module);

    // for some reason some classes arent seen without this o_O
    static std::vector<int> foo = {
        qMetaTypeId<model::DocumentNode*>(),
        qMetaTypeId<model::NamedColor*>(),
        qMetaTypeId<model::Bitmap*>(),
        qMetaTypeId<model::Gradient*>(),
    };

    define_io(glaxnimate_module);

    py::module model = glaxnimate_module.def_submodule("model", "");
    py::class_<model::Object, QObject>(model, "Object");

    py::class_<command::UndoMacroGuard>(model, "UndoMacroGuard")
        .def("__enter__", &command::UndoMacroGuard::start)
        .def("__exit__", [](command::UndoMacroGuard& g, pybind11::object, pybind11::object, pybind11::object){
            g.finish();
        })
        .def("start", &command::UndoMacroGuard::start)
        .def("finish", &command::UndoMacroGuard::finish)
        .attr("__doc__") = "Context manager that creates undo macros"
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
    register_from_meta<model::DocumentNode, model::Object>(model);
    register_from_meta<model::VisualNode, model::DocumentNode>(model);
    register_from_meta<model::AnimationContainer, model::Object>(model);
    register_from_meta<model::StretchableTime, model::Object>(model);
    register_from_meta<model::Transform, model::Object>(model);
    register_from_meta<model::MaskSettings, model::Object>(model);
    register_from_meta<model::Composition, model::VisualNode>(model)
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
    register_from_meta<model::detail::AnimatedPropertyBezier, model::AnimatableBase>(detail);
    register_animatable<math::bezier::Bezier, model::detail::AnimatedPropertyBezier>(detail);

    py::class_<PyVisitorPublic, PyVisitorTrampoline>(model, "Visitor")
        .def(py::init())
        .def("visit", (void (PyVisitorPublic::*)(model::Document*, bool))&PyVisitorPublic::visit, py::arg("document"), py::arg("skip_locked"))
        .def("visit", (void (PyVisitorPublic::*)(model::DocumentNode*, bool))&PyVisitorPublic::visit, py::arg("node"), py::arg("skip_locked"))
        .def("on_visit_document", &PyVisitorPublic::on_visit_document)
        .def("on_visit_node", &PyVisitorPublic::on_visit_node)
    ;

    py::module defs = model.def_submodule("assets", "");
    py::class_<model::AssetBase>(defs, "AssetBase")
        .def_property_readonly("users", &model::AssetBase::users)
    ;
    register_from_meta<model::Asset, model::DocumentNode, model::AssetBase>(defs);
    register_from_meta<model::BrushStyle, model::Asset>(defs);
    register_from_meta<model::NamedColor, model::BrushStyle>(defs);
    register_from_meta<model::GradientColors, model::Asset>(defs);
    register_from_meta<model::Gradient, model::BrushStyle>(defs, enums<model::Gradient::GradientType>{});
    register_from_meta<model::Bitmap, model::Asset>(defs);
    register_from_meta<model::Precomposition, model::Composition, model::AssetBase>(defs);
    register_from_meta<model::BitmapList, model::DocumentNode>(defs);
    register_from_meta<model::NamedColorList, model::DocumentNode>(defs);
    register_from_meta<model::GradientList, model::DocumentNode>(defs);
    register_from_meta<model::GradientColorsList, model::DocumentNode>(defs);
    register_from_meta<model::PrecompositionList, model::DocumentNode>(defs);
    register_from_meta<model::Assets, model::DocumentNode>(defs);

    py::module shapes = model.def_submodule("shapes", "");
    register_from_meta<model::ShapeElement, model::VisualNode>(shapes)
        .def("to_path", &model::ShapeElement::to_path)
    ;
    register_from_meta<model::Shape, model::ShapeElement>(shapes);
    register_from_meta<model::Modifier, model::ShapeElement>(shapes);
    register_from_meta<model::Styler, model::ShapeElement>(shapes);

    register_from_meta<model::Rect, model::Shape>(shapes);
    register_from_meta<model::Ellipse, model::Shape>(shapes);
    register_from_meta<model::PolyStar, model::Shape>(shapes, enums<model::PolyStar::StarType>{});
    register_from_meta<model::Path, model::Shape>(shapes);

    register_from_meta<model::Group, model::ShapeElement>(shapes)
        .def("add_shape", CreateObject(&model::Group::shapes), no_own,
            "Adds a shape from its class name",
             py::arg("type_name"),
             py::arg("index") = -1
        )
    ;
    register_from_meta<model::Layer, model::Group>(shapes);
    register_from_meta<model::PreCompLayer, model::ShapeElement>(shapes);

    register_from_meta<model::Fill, model::Styler>(shapes, enums<model::Fill::Rule>{});
    register_from_meta<model::Stroke, model::Styler>(shapes, enums<model::Stroke::Cap, model::Stroke::Join>{});

    register_from_meta<model::Image, model::ShapeElement>(shapes);

    register_from_meta<model::Repeater, model::Modifier>(shapes);
    register_from_meta<model::Trim, model::Modifier>(shapes);
}
