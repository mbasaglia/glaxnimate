#include "python_engine.hpp"

#include "model/document.hpp"
#include "ui/dialogs/glaxnimate_window.hpp"

#include "scripting/python/register_machinery.hpp"


scripting::ScriptEngine::Autoregister<scripting::python::PythonEngine> scripting::python::PythonEngine::autoreg;

static int counter = 0;

class scripting::python::PythonContext::Private
{
public:
    pybind11::module my_module;
    py::dict globals;
    py::function compile;
    const ScriptEngine* engine;
};

scripting::python::PythonContext::PythonContext(const ScriptEngine* engine)
{
    /// @note Not thread safe, pybind11 doesn't support multiple interpreters at once
    if ( counter == 0 )
        py::initialize_interpreter();
    counter++;

    d = std::make_unique<Private>();
    d->my_module = py::module::import("glaxnimate");
    d->globals = py::globals();
    d->compile = py::function(py::module(d->globals["__builtins__"]).attr("compile"));
    d->engine = engine;
}

scripting::python::PythonContext::~PythonContext()
{
    d.reset();

    counter--;
    if ( counter == 0 )
        py::finalize_interpreter();
}

void scripting::python::PythonContext::expose(const QString& name, const QVariant& obj)
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

class ModuleSetter
{
public:
    ModuleSetter(const QString& append)
    {
        sys = py::module::import("sys");
        python_path = py::list(sys.attr("path"));
        py::list python_path_new(sys.attr("path"));
        python_path_new.append(append);
        py::setattr(sys, "path", python_path_new);
    }

    ~ModuleSetter()
    {
        py::setattr(sys, "path", python_path);
    }

    py::module sys;
    py::list python_path;
};

bool scripting::python::PythonContext::run_from_module (
    const QDir& path,
    const QString& module,
    const QString& function,
    const QVariantList& args
)
{
    ModuleSetter{path.path()};

    try {
        py::module exec_module = py::module::import(module.toStdString().c_str());
        std::string std_func = function.toStdString();
        if ( !py::hasattr(exec_module, std_func.c_str()) )
            return false;

        py::tuple py_args(args.size());
        int i = 0;
        for ( const auto& arg : args )
            py_args[i++] = arg;
        exec_module.attr(std_func.c_str())(*py_args);
    } catch ( const py::error_already_set& pyexc ) {
        throw ScriptError(pyexc.what());
    }

    return true;
}

const scripting::ScriptEngine * scripting::python::PythonContext::engine() const
{
    return d->engine;
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
