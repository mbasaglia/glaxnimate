#include "python_engine.hpp"

#include "model/document.hpp"
#include "ui/dialogs/glaxnimate_window.hpp"

#include "scripting/python/register_machinery.hpp"


scripting::ScriptEngine::Autoregister<scripting::python::PythonEngine> scripting::python::PythonEngine::autoreg;


class scripting::python::PythonContext::Private
{
public:
    pybind11::module my_module;
    py::dict globals;
    py::function compile;
};

scripting::python::PythonContext::PythonContext()
{
    py::initialize_interpreter();
    d = std::make_unique<Private>();
    d->my_module = py::module::import("glaxnimate");
    d->globals = py::globals();
    d->compile = py::function(py::module(d->globals["__builtins__"]).attr("compile"));
}

scripting::python::PythonContext::~PythonContext()
{
    d.reset();
    py::finalize_interpreter();
}

void scripting::python::PythonContext::expose(const QString& name, QObject* obj)
{
    d->globals[name.toStdString().c_str()] = obj;
}

QString scripting::python::PythonContext::eval_to_string(const QString& code)
{

    std::string std_code = code.toStdString();
    bool eval = false;

    try {
        d->compile(std_code, "", "eval");
        eval = true;
    } catch ( const py::error_already_set& ) {}

    try {
        if ( eval )
            return QString::fromStdString(py::repr(py::eval(std_code)).cast<std::string>());
        py::exec(std_code);
        return {};
    } catch ( const py::error_already_set& pyexc ) {

        throw ScriptError(pyexc.what());
    }
}


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
    using namespace scripting::python;
    register_from_meta<GlaxnimateWindow, QObject>(m);
    register_from_meta<model::Document, QObject>(m);
    register_from_meta<model::DocumentNode, model::Object>(m);
    register_from_meta<model::Composition, model::DocumentNode>(m);
    register_from_meta<model::Animation, model::Composition>(m);
    register_from_meta<model::Layer, model::DocumentNode>(m);
    register_from_meta<model::ShapeLayer, model::Layer>(m);
    register_from_meta<model::EmptyLayer, model::Layer>(m);
}
