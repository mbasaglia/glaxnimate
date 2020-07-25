#include "register_machinery.hpp"

#include <QColor>
#include <QUuid>
#include <QVariant>


namespace scripting::python {

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
        arguments += 1;
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
                if ( len > 9 )
                    throw pybind11::value_error("Invalid argument count");

                ArgumentBuffer argbuf;

                argbuf.allocate_return_type<ReturnType>();

                for ( int i = 0; i < len; i++ )
                {
                   if ( !convert_argument(meth.parameterType(i), args[i], argbuf) )
                        throw pybind11::value_error("Invalid argument");
                }

                // Calling by name from QMetaObject ensures that default arguments work correctly
                bool ok = QMetaObject::invokeMethod(
                    o,
                    meth.name(),
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
                if ( !ok )
                    throw pybind11::value_error("Invalid method invocation");
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

} // namespace scripting::python
