#include "python_engine.hpp"

#include <cstring>

#include <QMetaProperty>
#include <QDebug>

#include "model/document.hpp"

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

        for ( int i = 0; i < meta.propertyCount(); i++ )
        {
            register_property(reg, meta.property(i));
        }

        return reg;
    }

private:
    template<class CppType>
    static void register_property_impl(PyClass& cls, const QMetaProperty& prop)
    {
        auto read = [prop](const Cls* o) { return qvariant_to_cpp<CppType>(prop.read(o)); };

        if ( prop.isWritable() )
            cls.def_property(prop.name(), read, [prop](Cls* o, const CppType& v) {
                prop.write(o, qvariant_from_cpp<CppType>(v));
            });
        else
            cls.def_property_readonly(prop.name(), read);
    }

    template<class CppType, class ThroughType>
    static void register_property_impl(PyClass& cls, const QMetaProperty& prop)
    {
        auto read = [prop](const Cls* o) { return qvariant_to_cpp<CppType>(prop.read(o)); };

        if ( prop.isWritable() )
            cls.def_property(prop.name(), read, [prop](Cls* o, const ThroughType& v) {
                prop.write(o, qvariant_from_cpp<CppType>(v));
            });
        else
            cls.def_property_readonly(prop.name(), read);
    }

    static void register_property(PyClass& cls, const QMetaProperty& prop)
    {
        if ( !prop.isScriptable() )
            return;

        if ( std::strcmp(prop.name(), "objectName") == 0 )
            return;

        switch ( QMetaType::Type(prop.type()) )
        {
            case QMetaType::Bool:       return register_property_impl<bool>(cls, prop);
            case QMetaType::Int:        return register_property_impl<int>(cls, prop);
            case QMetaType::UInt:       return register_property_impl<unsigned int>(cls, prop);
            case QMetaType::Double:     return register_property_impl<double>(cls, prop);
            case QMetaType::Long:       return register_property_impl<long>(cls, prop);
            case QMetaType::LongLong:   return register_property_impl<long long>(cls, prop);
            case QMetaType::Short:      return register_property_impl<short>(cls, prop);
            case QMetaType::ULong:      return register_property_impl<unsigned long>(cls, prop);
            case QMetaType::ULongLong:  return register_property_impl<unsigned long long>(cls, prop);
            case QMetaType::UShort:     return register_property_impl<unsigned short>(cls, prop);
            case QMetaType::Float:      return register_property_impl<float>(cls, prop);
            case QMetaType::QString:    return register_property_impl<std::string>(cls, prop);
            case QMetaType::QColor:     return register_property_impl<QColor>(cls, prop);
            case QMetaType::QUuid:      return register_property_impl<std::string>(cls, prop);
            case QMetaType::QObjectStar: return register_property_impl<QObject*>(cls, prop);
            case QMetaType::User:       return register_property_impl<QObject*>(cls, prop);
            case QMetaType::QVariantList: return register_property_impl<std::vector<QObject*>>(cls, prop);
            default:
                qWarning() << "Invalid property" << prop.name() << "of type" << prop.type() << prop.typeName();
        }
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
    QObjectBinder<model::Document, QObject>::register_from_meta(m);
    QObjectBinder<model::DocumentNode, model::Object>::register_from_meta(m);
    QObjectBinder<model::Composition, model::DocumentNode>::register_from_meta(m);
    QObjectBinder<model::Animation, model::Composition>::register_from_meta(m);
    QObjectBinder<model::Layer, model::DocumentNode>::register_from_meta(m);
    QObjectBinder<model::ShapeLayer, model::Layer>::register_from_meta(m);
    QObjectBinder<model::EmptyLayer, model::Layer>::register_from_meta(m);
}
