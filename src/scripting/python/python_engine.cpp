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

template<class Cls, class... Args>
class QObjectBinder
{
public:
    using PyClass = py::class_<Cls, Args...>;
    using CppClass = Cls;

    static PyClass register_from_meta(py::handle scope)
    {
        const QMetaObject& meta = CppClass::staticMetaObject;
        const char* name = meta.className();
        const char* clean_name = std::strrchr(name, ':');
        if ( clean_name == nullptr )
            clean_name = name;
        else
            clean_name++;

        PyClass reg(scope, clean_name);

        for ( int i = meta.superClass()->propertyCount(); i < meta.propertyCount(); i++ )
            register_property(reg, meta.property(i));

        for ( int  i = meta.superClass()->methodCount(); i < meta.methodCount(); i++ )
            register_method(reg, meta.method(i));


        return reg;
    }

private:
    template<class CppType>
        struct RegisterProperty
        {
            static void do_the_thing(PyClass& cls, const QMetaProperty& prop)
            {
                auto read = [prop](const Cls* o) { return qvariant_to_cpp<CppType>(prop.read(o)); };

                if ( prop.isWritable() )
                    cls.def_property(prop.name(), read, [prop](Cls* o, const CppType& v) {
                        prop.write(o, qvariant_from_cpp<CppType>(v));
                    });
                else
                    cls.def_property_readonly(prop.name(), read);
            }
        };


    template<template<class FuncT> class Func, class... FuncArgs>
    static bool type_dispatch(int meta_type, FuncArgs&&... args)
    {
        if ( meta_type >= QMetaType::User )
        {
            Func<QObject*>::do_the_thing(std::forward<FuncArgs>(args)...);
            return true;
        }

        switch ( QMetaType::Type(meta_type) )
        {
            case QMetaType::Bool:           Func<bool                    >::do_the_thing(std::forward<FuncArgs>(args)...); break;
            case QMetaType::Int:            Func<int                     >::do_the_thing(std::forward<FuncArgs>(args)...); break;
            case QMetaType::UInt:           Func<unsigned int            >::do_the_thing(std::forward<FuncArgs>(args)...); break;
            case QMetaType::Double:         Func<double                  >::do_the_thing(std::forward<FuncArgs>(args)...); break;
            case QMetaType::Long:           Func<long                    >::do_the_thing(std::forward<FuncArgs>(args)...); break;
            case QMetaType::LongLong:       Func<long long               >::do_the_thing(std::forward<FuncArgs>(args)...); break;
            case QMetaType::Short:          Func<short                   >::do_the_thing(std::forward<FuncArgs>(args)...); break;
            case QMetaType::ULong:          Func<unsigned long           >::do_the_thing(std::forward<FuncArgs>(args)...); break;
            case QMetaType::ULongLong:      Func<unsigned long long      >::do_the_thing(std::forward<FuncArgs>(args)...); break;
            case QMetaType::UShort:         Func<unsigned short          >::do_the_thing(std::forward<FuncArgs>(args)...); break;
            case QMetaType::Float:          Func<float                   >::do_the_thing(std::forward<FuncArgs>(args)...); break;
            case QMetaType::QString:        Func<std::string             >::do_the_thing(std::forward<FuncArgs>(args)...); break;
            case QMetaType::QColor:         Func<QColor                  >::do_the_thing(std::forward<FuncArgs>(args)...); break;
            case QMetaType::QUuid:          Func<std::string             >::do_the_thing(std::forward<FuncArgs>(args)...); break;
            case QMetaType::QObjectStar:    Func<QObject*                >::do_the_thing(std::forward<FuncArgs>(args)...); break;
            case QMetaType::QVariantList:   Func<std::vector<QObject*>   >::do_the_thing(std::forward<FuncArgs>(args)...); break;
            default:
                return false;
        }
        return true;
    }


    template<template<class FuncT> class Func, class... FuncArgs>
    static bool type_dispatch_maybe_void(int meta_type, FuncArgs&&... args)
    {
        if ( meta_type == QMetaType::Void )
        {
            Func<void>::do_the_thing(std::forward<FuncArgs>(args)...);
            return true;
        }
        return type_dispatch<Func>(meta_type, std::forward<FuncArgs>(args)...);
    }

    static void register_property(PyClass& cls, const QMetaProperty& prop)
    {
        if ( !prop.isScriptable() )
            return;

        if ( !type_dispatch<RegisterProperty>(prop.type(), cls, prop) )
            qWarning() << "Invalid property" << prop.name() << "of type" << prop.type() << prop.typeName();
    }


    template<class CppType>
        struct RegisterMethod0
        {
            static void do_the_thing(PyClass& cls, const QMetaMethod& meth)
            {
                cls.def(
                    meth.name(),
                    [meth](Cls* o) { return qvariant_to_cpp<CppType>(meth.invoke(o)); }
                );
            }
        };

    static void register_method_0(PyClass& cls, const QMetaMethod& meth)
    {
        if ( !type_dispatch_maybe_void<RegisterMethod0>(meth.returnType(), cls, meth) )
            qWarning() << "Invalid return type for " << meth.name() << ": " << meth.typeName();
    }


    template<class ReturnType>
        struct RegisterMethod1
        {
            template<class ArgType>
            struct RegisterMethod1Arg
            {
                static void do_the_thing(PyClass& cls, const QMetaMethod& meth)
                {
                    cls.def(
                        meth.name(),
                        [meth](Cls* o, const ArgType& arg) {
                            QVariant ret;
                            meth.invoke(
                                o,
                                Qt::DirectConnection,
                                Q_RETURN_ARG(QVariant, ret),
                                Q_ARG(QVariant, qvariant_from_cpp(arg))
                            );
                            return qvariant_to_cpp<ReturnType>(ret);
                        }
                    );
                }
            };

            static void do_the_thing(PyClass& cls, const QMetaMethod& meth)
            {
                if ( !type_dispatch<RegisterMethod1Arg>(meth.parameterType(0), cls, meth) )
                    qWarning() << "Invalid argument type for " << meth.name() << ": " << meth.parameterType(0);
            }
        };

    static void register_method_1(PyClass& cls, const QMetaMethod& meth)
    {
        if ( !type_dispatch_maybe_void<RegisterMethod1>(meth.returnType(), cls, meth) )
            qWarning() << "Invalid return type for " << meth.name() << ": " << meth.typeName();
    }

    static void register_method(PyClass& cls, const QMetaMethod& meth)
    {
        if ( meth.access() != QMetaMethod::Public )
            return;
        if ( meth.methodType() != QMetaMethod::Method && meth.methodType() != QMetaMethod::Slot )
            return;

        switch ( meth.parameterCount() )
        {
            case 0: return register_method_0(cls, meth);
            case 1: return register_method_1(cls, meth);
        }

        qDebug() << "Too many arguments for method " << meth.name() << ": " << meth.parameterCount();
    }
};

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
    QObjectBinder<GlaxnimateWindow, QObject>::register_from_meta(m);
    QObjectBinder<model::Document, QObject>::register_from_meta(m);
    QObjectBinder<model::DocumentNode, model::Object>::register_from_meta(m);
    QObjectBinder<model::Composition, model::DocumentNode>::register_from_meta(m);
    QObjectBinder<model::Animation, model::Composition>::register_from_meta(m);
    QObjectBinder<model::Layer, model::DocumentNode>::register_from_meta(m);
    QObjectBinder<model::ShapeLayer, model::Layer>::register_from_meta(m);
    QObjectBinder<model::EmptyLayer, model::Layer>::register_from_meta(m);
}
