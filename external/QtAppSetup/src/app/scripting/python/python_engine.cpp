#include "python_engine.hpp"

#include "app/scripting/python/register_machinery.hpp"
#include "app/log/log.hpp"
#include "app/env.hpp"


app::scripting::ScriptEngine::Autoregister<app::scripting::python::PythonEngine> app::scripting::python::PythonEngine::autoreg;

static int counter = 0;


class PY_HIDDEN CaptureStream
{
public:
    using OwnerT = app::scripting::ScriptExecutionContext;
    using SignalT = void (OwnerT::*)(const QString&);

    CaptureStream(OwnerT* owner = nullptr, SignalT signal = nullptr)
        : owner(owner), signal(signal)
    {}

    ~CaptureStream()
    {
        if ( owner )
        {
            py::delattr(stream, "write");
            py::delattr(stream, "flush");
        }
    }

    void setup(OwnerT* owner, SignalT signal, py::object dest)
    {
        try {
            dest.attr("write") = py::cpp_function([this](const QString& s){ write(s); });
            dest.attr("flush") = py::cpp_function([this](){ flush(); });
            stream = dest;
            this->owner = owner;
            this->signal = signal;
        } catch ( const py::error_already_set& pyexc ) {
            app::log::Log("Python").stream(app::log::Error)
                << "Could not initialize stream capture:" << pyexc.what();
        }
    }

    void write(const QString& data)
    {
        if ( data.isEmpty() )
            return;

        int from = 0;
        int to = data.indexOf('\n');
        while ( to != -1 )
        {
            QString txt;
            if ( !buf.isEmpty() )
            {
                txt = buf;
                buf.clear();
            }
            txt += data.mid(from, to-from);
            (owner->*signal)(txt);
            from = to+1;
            to = data.indexOf('\n', from);
        }
        buf += data.mid(from);
    }

    void flush()
    {
        (owner->*signal)(buf);
        buf.clear();
    }

private:
    OwnerT* owner;
    SignalT signal;
    QString buf;
    py::object stream;
};


class PY_HIDDEN app::scripting::python::PythonContext::Private
{
public:
    void init_capture(PythonContext* ctx)
    {
        sys = py::module::import("sys");
        py::object py_stdout = sys.attr("stdout");
        py::object py_stderr = sys.attr("stderr");

        if ( !py_stdout.is(py::none()) )
        {
            stdout_cap.setup(ctx, &PythonContext::stdout_line, py_stdout);
            stderr_cap.setup(ctx, &PythonContext::stderr_line, py_stderr);
        }

    }

    std::vector<pybind11::module> my_modules;
    py::dict globals;
    py::function compile;
    const ScriptEngine* engine;
    py::module sys;

    CaptureStream stderr_cap, stdout_cap;

};

app::scripting::python::PythonContext::PythonContext(const ScriptEngine* engine)
{
    /// @note Not thread safe, pybind11 doesn't support multiple interpreters at once
    if ( counter == 0 )
        py::initialize_interpreter();
    counter++;

    d = std::make_unique<Private>();
    d->globals = py::globals();
    d->compile = py::function(py::module(d->globals["__builtins__"]).attr("compile"));
    d->engine = engine;
    d->init_capture(this);
}

app::scripting::python::PythonContext::~PythonContext()
{
    d.reset();

    counter--;
    if ( counter == 0 )
        py::finalize_interpreter();
}

void app::scripting::python::PythonContext::expose(const QString& name, const QVariant& obj)
{
    try {
        d->globals[name.toStdString().c_str()] = obj;
    } catch ( const py::error_already_set& pyexc ) {
        throw ScriptError(pyexc.what());
    }
}

QString app::scripting::python::PythonContext::eval_to_string(const QString& code)
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
    } catch ( const py::builtin_exception& pyexc ) {
        throw ScriptError(pyexc.what());
    }
}

void app::scripting::python::PythonContext::app_module ( const QString& name )
{
    try {
        const char* cname = name.toStdString().c_str();
        d->my_modules.push_back(py::module::import(cname));
        d->globals[cname] = d->my_modules.back();
    } catch ( const py::error_already_set& pyexc ) {
        log::Log("Python", name).log(pyexc.what(), log::Error);
    }
}


class PY_HIDDEN ModuleSetter
{
public:
    ModuleSetter(py::module& sys, const QString& append) : sys(sys)
    {
        python_path = py::list(sys.attr("path"));
        py::list python_path_new(sys.attr("path"));
        python_path_new.append(append);
        py::setattr(sys, "path", python_path_new);
    }

    ~ModuleSetter()
    {
        py::setattr(sys, "path", python_path);
    }

    py::module& sys;
    py::list python_path;
};

bool app::scripting::python::PythonContext::run_from_module (
    const QDir& path,
    const QString& module,
    const QString& function,
    const QVariantList& args
)
{
    ModuleSetter{d->sys, path.path()};

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

const app::scripting::ScriptEngine * app::scripting::python::PythonContext::engine() const
{
    return d->engine;
}

app::scripting::ScriptContext app::scripting::python::PythonEngine::create_context() const
{
    return std::make_unique<PythonContext>(this);
}

void app::scripting::python::PythonEngine::add_module_search_paths(const QStringList& paths)
{
    app::Environment::Variable("PYTHONPATH").push_back(paths);
}
