#include "python_engine.hpp"

#include <cstring>

#include <QMetaProperty>
#include <QDebug>

#include "model/document.hpp"
#include "ui/dialogs/glaxnimate_window.hpp"

#undef slots
#include <pybind11/embed.h>
#include <pybind11/stl.h>


namespace py = pybind11;


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

namespace pybind11 { namespace detail {
    template <> struct type_caster<QString> {
    public:
        /**
         * This macro establishes the name 'QString' in
         * function signatures and declares a local variable
         * 'value' of type QString
         */
        PYBIND11_TYPE_CASTER(QString, _("QString"));

        /**
         * Conversion part 1 (Python->C++): convert a PyObject into a QString
         * instance or return false upon failure. The second argument
         * indicates whether implicit conversions should be applied.
         */
        bool load(handle src, bool ic)
        {
            type_caster<std::string> stdc;
            if ( stdc.load(src, ic) )
            {
                value = QString::fromStdString(stdc);
                return true;
            }
            return false;
        }

        /**
         * Conversion part 2 (C++ -> Python): convert an QString instance into
         * a Python object. The second and third arguments are used to
         * indicate the return value policy and parent object (for
         * ``return_value_policy::reference_internal``) and are generally
         * ignored by implicit casters.
         */
        static handle cast(QString src, return_value_policy policy, handle parent)
        {
            return type_caster<std::string>::cast(src.toStdString(), policy, parent);
        }
    };

    template <> struct type_caster<QUuid> {
    public:
        PYBIND11_TYPE_CASTER(QUuid, _("QUuid"));

        bool load(handle src, bool ic)
        {
            type_caster<QString> stdc;
            if ( stdc.load(src, ic) )
            {
                value = QUuid::fromString((const QString &)stdc);
                return true;
            }
            return false;
        }

        static handle cast(QUuid src, return_value_policy policy, handle parent)
        {
            return type_caster<QString>::cast(src.toString(), policy, parent);
        }
    };
}} // namespace pybind11::detail


template<class T> QVariant qvariant_from_cpp(const T& t) { return QVariant::fromValue(t); }
template<class T> T qvariant_to_cpp(const QVariant& v) { return v.value<T>(); }

template<> QVariant qvariant_from_cpp<std::string>(const std::string& t) { return QString::fromStdString(t); }
template<> std::string qvariant_to_cpp<std::string>(const QVariant& v) { return v.toString().toStdString(); }

template<> void qvariant_to_cpp<void>(const QVariant&) {}


template<>
QVariant qvariant_from_cpp<std::vector<QObject*>>(const std::vector<QObject*>& t)
{
    QVariantList list;
    for ( QObject* obj : t )
        list.push_back(QVariant::fromValue(obj));
    return list;

}
template<> std::vector<QObject*> qvariant_to_cpp<std::vector<QObject*>>(const QVariant& v)
{
    std::vector<QObject*> objects;
    for ( const QVariant& vi : v.toList() )
        objects.push_back(vi.value<QObject*>());
    return objects;
}


template<template<class FuncT> class Func, class RetT, class... FuncArgs>
RetT type_dispatch(int meta_type, FuncArgs&&... args)
{
    if ( meta_type >= QMetaType::User )
    {
        return Func<QObject*>::do_the_thing(std::forward<FuncArgs>(args)...);
    }

    switch ( QMetaType::Type(meta_type) )
    {
        case QMetaType::Bool:           return Func<bool                    >::do_the_thing(std::forward<FuncArgs>(args)...);
        case QMetaType::Int:            return Func<int                     >::do_the_thing(std::forward<FuncArgs>(args)...);
//         case QMetaType::UInt:           return Func<unsigned int            >::do_the_thing(std::forward<FuncArgs>(args)...);
        case QMetaType::Double:         return Func<double                  >::do_the_thing(std::forward<FuncArgs>(args)...);
//         case QMetaType::Long:           return Func<long                    >::do_the_thing(std::forward<FuncArgs>(args)...);
//         case QMetaType::LongLong:       return Func<long long               >::do_the_thing(std::forward<FuncArgs>(args)...);
//         case QMetaType::Short:          return Func<short                   >::do_the_thing(std::forward<FuncArgs>(args)...);
//         case QMetaType::ULong:          return Func<unsigned long           >::do_the_thing(std::forward<FuncArgs>(args)...);
//         case QMetaType::ULongLong:      return Func<unsigned long long      >::do_the_thing(std::forward<FuncArgs>(args)...);
//         case QMetaType::UShort:         return Func<unsigned short          >::do_the_thing(std::forward<FuncArgs>(args)...);
        case QMetaType::Float:          return Func<float                   >::do_the_thing(std::forward<FuncArgs>(args)...);
        case QMetaType::QString:        return Func<QString                 >::do_the_thing(std::forward<FuncArgs>(args)...);
        case QMetaType::QColor:         return Func<QColor                  >::do_the_thing(std::forward<FuncArgs>(args)...);
        case QMetaType::QUuid:          return Func<QUuid                   >::do_the_thing(std::forward<FuncArgs>(args)...);
        case QMetaType::QObjectStar:    return Func<QObject*                >::do_the_thing(std::forward<FuncArgs>(args)...);
        case QMetaType::QVariantList:   return Func<std::vector<QObject*>   >::do_the_thing(std::forward<FuncArgs>(args)...);
        default:
            return RetT{};
    }
}

template<template<class FuncT> class Func, class RetT, class... FuncArgs>
static RetT type_dispatch_maybe_void(int meta_type, FuncArgs&&... args)
{
    if ( meta_type == QMetaType::Void )
        return Func<void>::do_the_thing(std::forward<FuncArgs>(args)...);
    return type_dispatch<Func, RetT>(meta_type, std::forward<FuncArgs>(args)...);
}


struct PyPropertyInfo
{
    const char* name = nullptr;
    py::cpp_function get;
    py::cpp_function set;
};


template<class CppType>
    struct RegisterProperty
    {
        static PyPropertyInfo do_the_thing(const QMetaProperty& prop)
        {
            PyPropertyInfo py;
            py.name = prop.name();
            py.get = py::cpp_function(
                [prop](const QObject* o) { return qvariant_to_cpp<CppType>(prop.read(o)); },
                py::return_value_policy::automatic_reference
            );

            if ( prop.isWritable() )
                py.set = py::cpp_function([prop](QObject* o, const CppType& v) {
                    prop.write(o, qvariant_from_cpp<CppType>(v));
                });
            return py;
        }
    };

PyPropertyInfo register_property(const QMetaProperty& prop)
{
    if ( !prop.isScriptable() )
        return {};

    PyPropertyInfo pyprop = type_dispatch<RegisterProperty, PyPropertyInfo>(prop.type(), prop);
    if ( !pyprop.name )
        qWarning() << "Invalid property" << prop.name() << "of type" << prop.type() << prop.typeName();
    return pyprop;
}

template<class T> const char* type_name();
#define TYPE_NAME(T) template<> const char* type_name<T>() { return #T; }
TYPE_NAME(int)
TYPE_NAME(bool)
TYPE_NAME(double)
TYPE_NAME(float)
TYPE_NAME(QString)
TYPE_NAME(QColor)
TYPE_NAME(QUuid)
TYPE_NAME(QObject*)
TYPE_NAME(QVariantList)
TYPE_NAME(std::vector<QObject*>)


class ArgumentBuffer
{
public:
    ArgumentBuffer() = default;
    ArgumentBuffer(const ArgumentBuffer&) = delete;
    ArgumentBuffer& operator=(const ArgumentBuffer&) = delete;
    ~ArgumentBuffer()
    {
        for ( const auto& d : destructors )
            d();
    }

    template<class CppType>
    CppType* allocate()
    {
        if ( avail() < int(sizeof(CppType)) )
            throw py::type_error("Cannot allocate argument");

        CppType* addr = new (next_mem()) CppType;
        buffer_used += sizeof(CppType);
        generic_args[arguments] = { type_name<CppType>(), addr };
        ensure_destruction(addr);
        return addr;
    }

    template<class CppType>
    void allocate_return_type()
    {
        if ( avail() < int(sizeof(CppType)) )
            throw py::type_error("Cannot allocate return value");

        CppType* addr = new (next_mem()) CppType;
        buffer_used += sizeof(CppType);
        ret = { type_name<CppType>(), addr };
        ensure_destruction(addr);
        ret_addr = addr;
    }

    template<class CppType>
    CppType return_value()
    {
        return *static_cast<CppType*>(ret_addr);
    }

    const QGenericArgument& arg(int i) const { return generic_args[i]; }

    const QGenericReturnArgument& return_arg() const { return ret; }

private:
    int arguments = 0;
    int buffer_used = 0;
    std::array<char, 128> buffer;
    std::vector<std::function<void()>> destructors;
    std::array<QGenericArgument, 9> generic_args;
    QGenericReturnArgument ret;
    void* ret_addr = nullptr;


    int avail() { return buffer.size() - buffer_used; }
    void* next_mem() { return &buffer + buffer_used; }


    template<class CppType>
        std::enable_if_t<std::is_pod_v<CppType>> ensure_destruction(CppType*) {}


    template<class CppType>
        std::enable_if_t<!std::is_pod_v<CppType>> ensure_destruction(CppType* addr)
        {
           destructors.push_back([addr]{ addr->~CppType(); });
        }
};

template<> void ArgumentBuffer::allocate_return_type<void>(){}
template<> void ArgumentBuffer::return_value<void>(){}


template<class CppType>
    struct ConvertArgument
    {
        static bool do_the_thing(const py::handle& val, ArgumentBuffer& buf)
        {
            *buf.allocate<CppType>() = val.cast<CppType>();
            return true;
        }
    };

bool convert_argument(int meta_type, const py::handle& value, ArgumentBuffer& buf)
{
    return type_dispatch<ConvertArgument, bool>(meta_type, value, buf);
}


struct PyMethodInfo
{
    const char* name = nullptr;
    py::cpp_function method;
};

template<class ReturnType>
struct RegisterMethod
{
    static PyMethodInfo do_the_thing(const QMetaMethod& meth, py::handle& handle)
    {
        PyMethodInfo py;
        py.name = meth.name();
        py.method = py::cpp_function(
            [meth](QObject* o, py::args args)
            {
                int len = py::len(args);
                if ( len > 9 || len != meth.parameterCount() )
                    throw pybind11::value_error("Invalid argument count");

                ArgumentBuffer argbuf;

                for ( int i = 0; i < len; i++ )
                {
                   if ( !convert_argument(meth.parameterType(i), args[i], argbuf) )
                        throw pybind11::value_error("Invalid argument");
                }

                argbuf.allocate_return_type<ReturnType>();

                meth.invoke(
                    o,
                    Qt::DirectConnection,
                    argbuf.return_arg(),
                    argbuf.arg(0),
                    argbuf.arg(1),
                    argbuf.arg(2),
                    argbuf.arg(3),
                    argbuf.arg(4),
                    argbuf.arg(5),
                    argbuf.arg(6),
                    argbuf.arg(7),
                    argbuf.arg(8),
                    argbuf.arg(9)
                );
                return argbuf.return_value<ReturnType>();
            },
            py::name(py.name),
            py::is_method(handle),
            py::sibling(py::getattr(handle, py.name, py::none())),
            py::return_value_policy::automatic_reference
        );
        return py;
    }
};

PyMethodInfo register_method(const QMetaMethod& meth, py::handle& handle)
{
    if ( meth.access() != QMetaMethod::Public )
        return {};
    if ( meth.methodType() != QMetaMethod::Method && meth.methodType() != QMetaMethod::Slot )
        return {};

    if ( meth.parameterCount() > 9 )
    {
        qDebug() << "Too many arguments for method " << meth.name() << ": " << meth.parameterCount();
        return {};
    }

    PyMethodInfo pymeth = type_dispatch_maybe_void<RegisterMethod, PyMethodInfo>(meth.returnType(), meth, handle);
    if ( !pymeth.name )
        qWarning() << "Invalid method" << meth.name() << "return type" << meth.returnType() << meth.typeName();
    return pymeth;

}

template<class CppClass, class... Args>
py::class_<CppClass, Args...> register_from_meta(py::handle scope)
{
    const QMetaObject& meta = CppClass::staticMetaObject;
    const char* name = meta.className();
    const char* clean_name = std::strrchr(name, ':');
    if ( clean_name == nullptr )
        clean_name = name;
    else
        clean_name++;

    py::class_<CppClass, Args...> reg(scope, clean_name);

    for ( int i = meta.superClass()->propertyCount(); i < meta.propertyCount(); i++ )
    {
        PyPropertyInfo pyprop = register_property(meta.property(i));
        if ( pyprop.name )
            reg.def_property(pyprop.name, pyprop.get, pyprop.set);
    }

    for ( int  i = meta.superClass()->methodCount(); i < meta.methodCount(); i++ )
    {
        PyMethodInfo pymeth = register_method(meta.method(i), reg);
        if ( pymeth.name )
            reg.attr(pymeth.name) = pymeth.method;
    }


    return reg;
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
    register_from_meta<GlaxnimateWindow, QObject>(m);
    register_from_meta<model::Document, QObject>(m);
    register_from_meta<model::DocumentNode, model::Object>(m);
    register_from_meta<model::Composition, model::DocumentNode>(m);
    register_from_meta<model::Animation, model::Composition>(m);
    register_from_meta<model::Layer, model::DocumentNode>(m);
    register_from_meta<model::ShapeLayer, model::Layer>(m);
    register_from_meta<model::EmptyLayer, model::Layer>(m);
}
