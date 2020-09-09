#pragma once

#include <cstring>

#include <QMetaProperty>
#include <QMetaEnum>
#include <QDebug>

#include "app/scripting/python/casters.hpp"

namespace py = pybind11;

namespace app::scripting::python {

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

struct PyEnumInfo
{
    const char* name = nullptr;
    py::handle enum_handle;
};

PyPropertyInfo register_property(const QMetaProperty& prop);
PyMethodInfo register_method(const QMetaMethod& meth, py::handle& handle);

template<class EnumT>
PyEnumInfo register_enum(const QMetaEnum& meta, py::handle& scope)
{
    py::enum_<EnumT> pyenum(scope, meta.name());
    for ( int i = 0; i < meta.keyCount(); i++ )
        pyenum.value(meta.key(i), EnumT(meta.value(i)));

    return {meta.name(), pyenum};
}


template<class... Enums>
struct enums;

template<class EnumT, class... Others>
struct enums<EnumT, Others...>
    : public enums<Others...>
{
    void process(py::handle& scope, std::vector<PyEnumInfo>& out)
    {
        out.push_back(register_enum<EnumT>(QMetaEnum::fromType<EnumT>(), scope));
        enums<Others...>::process(scope, out);
    }
};

template<>
struct enums<>
{
    void process(py::handle&, std::vector<PyEnumInfo>&) {}
};

template<class CppClass, class... Args, class... Enums>
py::class_<CppClass, Args...> register_from_meta(py::handle scope, enums<Enums...> reg_enums = {})
{
    const QMetaObject& meta = CppClass::staticMetaObject;
    const char* name = meta.className();
    const char* clean_name = std::strrchr(name, ':');
    if ( clean_name == nullptr )
        clean_name = name;
    else
        clean_name++;

    py::class_<CppClass, Args...> reg(scope, clean_name);

    auto super = meta.superClass();

    for ( int i = !super ? 0 : super->propertyCount(); i < meta.propertyCount(); i++ )
    {
        PyPropertyInfo pyprop = register_property(meta.property(i));
        if ( pyprop.name )
            reg.def_property(pyprop.name, pyprop.get, pyprop.set);
    }

    for ( int i = !super ? 0 : super->methodCount(); i < meta.methodCount(); i++ )
    {
        PyMethodInfo pymeth = register_method(meta.method(i), reg);
        if ( pymeth.name )
            reg.attr(pymeth.name) = pymeth.method;
    }

    std::vector<PyEnumInfo> enum_info;
    reg_enums.process(reg, enum_info);
    for ( const auto& info : enum_info )
        reg.attr(info.name) = info.enum_handle;

    return reg;
}

namespace detail {

template<class T>
QString qdebug_operator_to_string(const T& o)
{
    QString data;
    QDebug(&data) << o;
    return data;
}

} // namespace detail

template<class T>
auto qdebug_operator_to_string()
{
    return &detail::qdebug_operator_to_string<T>;
}

} // namespace app::scripting::python
