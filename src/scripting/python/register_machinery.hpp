#pragma once

#include <cstring>

#include <QMetaProperty>
#include <QDebug>

#include "scripting/python/casters.hpp"

namespace py = pybind11;

namespace scripting::python {



struct PyPropertyInfo
{
    const char* name = nullptr;
    py::cpp_function get;
    py::cpp_function set;
};

struct PyMethodInfo
{
    const char* name = nullptr;
    py::cpp_function method;
};

PyPropertyInfo register_property(const QMetaProperty& prop);
PyMethodInfo register_method(const QMetaMethod& meth, py::handle& handle);

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


} // namespace scripting::python
