#include "register_machinery.hpp"

#include <utility>

#include <QColor>
#include <QUuid>
#include <QVariant>
#include <QPointF>
#include <QSizeF>
#include <QVector2D>
#include <QRectF>
#include <QByteArray>
#include <QDateTime>
#include <QDate>
#include <QTime>
#include <QGradient>

#include "app/log/log.hpp"


namespace app::scripting::python {

template<class T> const char* type_name() { return QMetaType::typeName(qMetaTypeId<T>()); }
template<int> struct meta_2_cpp_s;
template<class> struct cpp_2_meta_s;

#define TYPE_NAME(Type) //template<> const char* type_name<Type>() { return #Type; }
#define SETUP_TYPE(MetaInt, Type)                                   \
    TYPE_NAME(Type)                                                 \
    template<> struct meta_2_cpp_s<MetaInt> { using type = Type; }; \
    template<> struct cpp_2_meta_s<Type> { static constexpr const int value = MetaInt; };

template<int meta_type> using meta_2_cpp = typename meta_2_cpp_s<meta_type>::type;
template<class T> constexpr const int cpp_2_meta = cpp_2_meta_s<T>::value;

SETUP_TYPE(QMetaType::Int,          int)
SETUP_TYPE(QMetaType::Bool,         bool)
SETUP_TYPE(QMetaType::Double,       double)
SETUP_TYPE(QMetaType::Float,        float)
SETUP_TYPE(QMetaType::UInt,         unsigned int)
SETUP_TYPE(QMetaType::Long,         long)
SETUP_TYPE(QMetaType::LongLong,     long long)
SETUP_TYPE(QMetaType::Short,        short)
SETUP_TYPE(QMetaType::ULong,        unsigned long)
SETUP_TYPE(QMetaType::ULongLong,    unsigned long long)
SETUP_TYPE(QMetaType::UShort,       unsigned short)
SETUP_TYPE(QMetaType::QString,      QString)
SETUP_TYPE(QMetaType::QColor,       QColor)
SETUP_TYPE(QMetaType::QUuid,        QUuid)
SETUP_TYPE(QMetaType::QObjectStar,  QObject*)
SETUP_TYPE(QMetaType::QVariantList, QVariantList)
SETUP_TYPE(QMetaType::QVariant,     QVariant)
SETUP_TYPE(QMetaType::QStringList,  QStringList)
SETUP_TYPE(QMetaType::QVariantMap,  QVariantMap)
SETUP_TYPE(QMetaType::QVariantHash, QVariantHash)
SETUP_TYPE(QMetaType::QPointF,      QPointF)
SETUP_TYPE(QMetaType::QSizeF,       QSizeF)
SETUP_TYPE(QMetaType::QSize,        QSize)
SETUP_TYPE(QMetaType::QVector2D,    QVector2D)
SETUP_TYPE(QMetaType::QRectF,       QRectF)
SETUP_TYPE(QMetaType::QByteArray,   QByteArray)
SETUP_TYPE(QMetaType::QDateTime,    QDateTime)
SETUP_TYPE(QMetaType::QDate,        QDate)
SETUP_TYPE(QMetaType::QTime,        QTime)
SETUP_TYPE(QMetaType::QImage,       QImage)
// If you add stuff here, remember to add it to supported_types too

TYPE_NAME(std::vector<QObject*>)

using supported_types = std::integer_sequence<int,
    QMetaType::Bool,
    QMetaType::Int,
    QMetaType::Double,
    QMetaType::Float,
    QMetaType::UInt,
    QMetaType::Long,
    QMetaType::LongLong,
    QMetaType::Short,
    QMetaType::ULong,
    QMetaType::ULongLong,
    QMetaType::UShort,
    QMetaType::QString,
    QMetaType::QColor,
    QMetaType::QUuid,
    QMetaType::QObjectStar,
    QMetaType::QVariantList,
    QMetaType::QVariant,
    QMetaType::QStringList,
    QMetaType::QVariantMap,
    QMetaType::QVariantHash,
    QMetaType::QPointF,
    QMetaType::QSizeF,
    QMetaType::QSize,
    QMetaType::QVector2D,
    QMetaType::QRectF,
    QMetaType::QByteArray,
    QMetaType::QDateTime,
    QMetaType::QDate,
    QMetaType::QTime,
    QMetaType::QImage
    // Ensure new types have SETUP_TYPE
>;

template<class T> QVariant qvariant_from_cpp(const T& t) { return QVariant::fromValue(t); }
template<class T> T qvariant_to_cpp(const QVariant& v) { return v.value<T>(); }

template<> QVariant qvariant_from_cpp<std::string>(const std::string& t) { return QString::fromStdString(t); }
template<> std::string qvariant_to_cpp<std::string>(const QVariant& v) { return v.toString().toStdString(); }

template<> QVariant qvariant_from_cpp<QVariant>(const QVariant& t) { return t; }
template<> QVariant qvariant_to_cpp<QVariant>(const QVariant& v) { return v; }


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

template<int i, template<class FuncT> class Func, class RetT, class... FuncArgs>
bool type_dispatch_impl_step(int meta_type, RetT& ret, FuncArgs&&... args)
{
    if ( meta_type != i )
        return false;

    ret = Func<meta_2_cpp<i>>::do_the_thing(std::forward<FuncArgs>(args)...);
    return true;
}

template<template<class FuncT> class Func, class RetT, class... FuncArgs, int... i>
bool type_dispatch_impl(int meta_type, RetT& ret, std::integer_sequence<int, i...>, FuncArgs&&... args)
{
    return (type_dispatch_impl_step<i, Func>(meta_type, ret, std::forward<FuncArgs>(args)...)||...);
}

template<template<class FuncT> class Func, class RetT, class... FuncArgs>
RetT type_dispatch(int meta_type, FuncArgs&&... args)
{
    if ( meta_type >= QMetaType::User )
    {
        if ( QMetaType(meta_type).flags() & QMetaType::IsEnumeration )
            return Func<int>::do_the_thing(std::forward<FuncArgs>(args)...);
        return Func<QObject*>::do_the_thing(std::forward<FuncArgs>(args)...);
    }
    RetT ret;
    type_dispatch_impl<Func>(meta_type, ret, supported_types(), std::forward<FuncArgs>(args)...);
    return ret;
}

template<template<class FuncT> class Func, class RetT, class... FuncArgs>
static RetT type_dispatch_maybe_void(int meta_type, FuncArgs&&... args)
{
    if ( meta_type == QMetaType::Void )
        return Func<void>::do_the_thing(std::forward<FuncArgs>(args)...);
    return type_dispatch<Func, RetT>(meta_type, std::forward<FuncArgs>(args)...);
}

std::string fix_type(QByteArray ba)
{
    if ( ba.endsWith('*') || ba.endsWith('&') )
        ba.remove(ba.size()-1, 1);

    if ( ba.startsWith("const ") )
        ba.remove(0, 6);

    if ( ba == "QString" )
        return "str";
    else if ( ba == "QVariantList" )
        return "list";
    if ( ba == "QStringList" )
        return "List[str]";
    else if ( ba == "double" )
        return "float";
    else if ( ba == "void" )
        return "None";
    else if ( ba == "QImage" )
        return "PIL.Image.Image";

    return ba.toStdString();
}


template<class CppType>
    struct RegisterProperty
    {
        static PyPropertyInfo do_the_thing(const QMetaProperty& prop)
        {
            PyPropertyInfo py;
            py.name = prop.name();
            std::string sig = "Type: " + fix_type(prop.typeName());
            py.get = py::cpp_function(
                [prop](const QObject* o) { return qvariant_to_cpp<CppType>(prop.read(o)); },
                py::return_value_policy::automatic_reference,
                sig.c_str()
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

    PyPropertyInfo pyprop = type_dispatch<RegisterProperty, PyPropertyInfo>(prop.userType(), prop);
    if ( !pyprop.name )
        log::LogStream("Python", "", log::Error) << "Invalid property" << prop.name() << "of type" << prop.userType() << prop.typeName();
    return pyprop;
}

class ArgumentBuffer
{
public:
    ArgumentBuffer(const QMetaMethod& method) : method(method) {}
    ArgumentBuffer(const ArgumentBuffer&) = delete;
    ArgumentBuffer& operator=(const ArgumentBuffer&) = delete;
    ~ArgumentBuffer()
    {
        for ( int i = 0; i < destructors_used; i++)
        {
            destructors[i]->destruct();
            delete destructors[i];
        }
    }

    template<class CppType>
    const char* object_type_name(const CppType&)
    {
        return type_name<CppType>();
    }

    std::string object_type_name(QObject* value)
    {
        std::string s = value->metaObject()->className();
        std::string target = method.parameterTypes()[arguments].toStdString();
        if ( !target.empty() && target.back() == '*' )
        {
            target.pop_back();
            if ( s != target )
            {
                for ( auto mo = value->metaObject()->superClass(); mo; mo = mo->superClass() )
                {
                    std::string moname = mo->className();
                    if ( moname == target )
                        return target + "*";
                }
            }
        }
        return s + "*";
    }

    template<class CppType>
    CppType* allocate(const CppType& value)
    {
        if ( avail() < int(sizeof(CppType)) )
            throw py::type_error("Cannot allocate argument");

        CppType* addr = new (next_mem()) CppType;
        buffer_used += sizeof(CppType);
        names[arguments] = object_type_name(value);
        generic_args[arguments] = { names[arguments].c_str(), addr };
        ensure_destruction(addr);
        arguments += 1;
        *addr = value;
        return addr;
    }

    template<class CppType>
    void allocate_return_type(const char* name)
    {
        if ( avail() < int(sizeof(CppType)) )
            throw py::type_error("Cannot allocate return value");

        CppType* addr = new (next_mem()) CppType;
        buffer_used += sizeof(CppType);
        ret = { name, addr };
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
    class Destructor
    {
    public:
        Destructor() = default;
        Destructor(const Destructor&) = delete;
        Destructor& operator=(const Destructor&) = delete;
        virtual ~Destructor() = default;
        virtual void destruct() const = 0;
    };

    template<class CppType>
    struct DestructorImpl : public Destructor
    {
        DestructorImpl(CppType* addr) : addr(addr) {}

        void destruct() const override
        {
            addr->~CppType();
        }

        CppType* addr;
    };

    int arguments = 0;
    int buffer_used = 0;
    std::array<char, 128> buffer;
    std::array<Destructor*, 9> destructors;
    std::array<QGenericArgument, 9> generic_args;
    std::array<std::string, 9> names;
    QGenericReturnArgument ret;
    void* ret_addr = nullptr;
    QMetaMethod method;
    int destructors_used = 0;


    int avail() { return buffer.size() - buffer_used; }
    void* next_mem() { return buffer.data() + buffer_used; }


    template<class CppType>
        std::enable_if_t<std::is_pod_v<CppType>> ensure_destruction(CppType*) {}


    template<class CppType>
        std::enable_if_t<!std::is_pod_v<CppType>> ensure_destruction(CppType* addr)
        {
            destructors[destructors_used] = new DestructorImpl<CppType>(addr);
            destructors_used++;
        }
};

template<> void ArgumentBuffer::allocate_return_type<void>(const char*){}
template<> void ArgumentBuffer::return_value<void>(){}


template<class CppType>
    struct ConvertArgument
    {
        static bool do_the_thing(const py::handle& val, ArgumentBuffer& buf)
        {
            buf.allocate<CppType>(val.cast<CppType>());
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

        std::string signature = "Signature:\n";
        signature += meth.name().toStdString();
        signature += "(self";
        auto names = meth.parameterNames();
        auto types = meth.parameterTypes();
        for ( int i = 0; i < meth.parameterCount(); i++ )
        {
            signature += ", ";
            signature += names[i].toStdString();
            signature += ": ";
            signature += fix_type(types[i]);
        }
        signature += ") -> ";
        signature += fix_type(meth.typeName());

        py.method = py::cpp_function(
            [meth](QObject* o, py::args args) -> ReturnType
            {
                int len = py::len(args);
                if ( len > 9 )
                    throw pybind11::value_error("Invalid argument count");

                ArgumentBuffer argbuf(meth);

                argbuf.allocate_return_type<ReturnType>(meth.typeName());

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
            py::return_value_policy::automatic_reference,
            signature.c_str()
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
        log::LogStream("Python", "", log::Error) << "Too many arguments for method " << meth.name() << ": " << meth.parameterCount();
        return {};
    }

    PyMethodInfo pymeth = type_dispatch_maybe_void<RegisterMethod, PyMethodInfo>(meth.returnType(), meth, handle);
    if ( !pymeth.name )
        log::LogStream("Python", "", log::Error) << "Invalid method" << meth.name() << "return type" << meth.returnType() << meth.typeName();
    return pymeth;

}

template<int i>
bool qvariant_type_caster_load_impl(QVariant& into, const pybind11::handle& src)
{
    auto caster = pybind11::detail::make_caster<meta_2_cpp<i>>();
    if ( caster.load(src, false) )
    {
        into = QVariant::fromValue(pybind11::detail::cast_op<meta_2_cpp<i>>(caster));
        return true;
    }
    return false;
}

template<>
bool qvariant_type_caster_load_impl<QMetaType::QVariant>(QVariant&, const pybind11::handle&) { return false; }

template<int... i>
bool qvariant_type_caster_load(QVariant& into, const pybind11::handle& src, std::integer_sequence<int, i...>)
{
    return (qvariant_type_caster_load_impl<i>(into, src)||...);
}

template<int i>
bool qvariant_type_caster_cast_impl(
    pybind11::handle& into, const QVariant& src,
    pybind11::return_value_policy policy, const pybind11::handle& parent)
{
    if ( src.type() == i )
    {
        into = pybind11::detail::make_caster<meta_2_cpp<i>>::cast(src.value<meta_2_cpp<i>>(), policy, parent);
        return true;
    }
    return false;
}

template<>
bool qvariant_type_caster_cast_impl<QMetaType::QVariant>(
    pybind11::handle&, const QVariant&, pybind11::return_value_policy, const pybind11::handle&)
{
    return false;
}


template<int... i>
bool qvariant_type_caster_cast(
    pybind11::handle& into,
    const QVariant& src,
    pybind11::return_value_policy policy,
    const pybind11::handle& parent,
    std::integer_sequence<int, i...>
)
{
    return (qvariant_type_caster_cast_impl<i>(into, src, policy, parent)||...);
}

} // namespace app::scripting::python

using namespace app::scripting::python;

bool pybind11::detail::type_caster<QVariant>::load(handle src, bool)
{
    if ( src.ptr() == Py_None )
    {
        value = QVariant();
        return true;
    }
    return qvariant_type_caster_load(value, src, supported_types());
}

pybind11::handle pybind11::detail::type_caster<QVariant>::cast(QVariant src, return_value_policy policy, handle parent)
{
    if ( src.isNull() )
        return pybind11::none();

    policy = py::return_value_policy::automatic_reference;

    int meta_type = src.userType();
    if ( meta_type >= QMetaType::User )
    {
        if ( QMetaType(meta_type).flags() & QMetaType::IsEnumeration )
            return pybind11::detail::make_caster<int>::cast(src.value<int>(), policy, parent);
        else if ( meta_type == qMetaTypeId<QGradientStops>() )
            return pybind11::detail::make_caster<QGradientStops>::cast(src.value<QGradientStops>(), policy, parent);
        return pybind11::detail::make_caster<QObject*>::cast(src.value<QObject*>(), policy, parent);
    }

    pybind11::handle ret;
    qvariant_type_caster_cast(ret, src, policy, parent, supported_types());
    return ret;
}


bool pybind11::detail::type_caster<QUuid>::load(handle src, bool ic)
{
    if ( isinstance(src, pybind11::module_::import("uuid").attr("UUID")) )
        src = py::str(src);
    type_caster<QString> stdc;
    if ( stdc.load(src, ic) )
    {
        value = QUuid::fromString((const QString &)stdc);
        return true;
    }
    return false;
}

pybind11::handle pybind11::detail::type_caster<QUuid>::cast(QUuid src, return_value_policy policy, handle parent)
{
    auto str = type_caster<QString>::cast(src.toString(), policy, parent);
    return pybind11::module_::import("uuid").attr("UUID")(str).release();
}



bool pybind11::detail::type_caster<QImage>::load(handle src, bool)
{
    if ( !isinstance(src, pybind11::module_::import("PIL.Image").attr("Image")) )
        return false;

    py::object obj = py::reinterpret_borrow<py::object>(src);
    std::string mode = obj.attr("mode").cast<std::string>();
    QImage::Format format;
    if ( mode == "RGBA" )
    {
        format = QImage::Format_RGBA8888;
    }
    else if ( mode == "RGB" )
    {
        format = QImage::Format_RGB888;
    }
    else if ( mode == "RGBa" )
    {
        format = QImage::Format_RGBA8888_Premultiplied;
    }
    else if ( mode == "RGBX" )
    {
        format = QImage::Format_RGBX8888;
    }
    else
    {
        format = QImage::Format_RGBA8888;
        obj = obj.attr("convert")("RGBA");
    }

    std::string data = obj.attr("tobytes")().cast<std::string>();

    int width = obj.attr("width").cast<int>();
    int height = obj.attr("height").cast<int>();
    value = QImage(width, height, format);
    if ( data.size() != std::size_t(value.sizeInBytes()) )
        return false;
    std::memcpy(value.bits(), data.data(), data.size());
    return true;
}

pybind11::handle pybind11::detail::type_caster<QImage>::cast(QImage src, return_value_policy, handle)
{
    auto mod = pybind11::module_::import("PIL.Image");
    auto frombytes = mod.attr("frombytes");

    const char* mode;

    switch ( src.format() )
    {
        case QImage::Format_Invalid:
            return mod.attr("Image")().release();
        case QImage::Format_RGB888:
            mode = "RGB";
            break;
        case QImage::Format_RGBA8888:
            mode = "RGBA";
            break;
        case QImage::Format_RGBA8888_Premultiplied:
            mode = "RGBa";
            break;
        case QImage::Format_RGBX8888:
            mode = "RGBX";
            break;
        default:
            mode = "RGBA";
            src = src.convertToFormat(QImage::Format_RGBA8888);
            break;
    }

    py::tuple size(2);
    size[0] = py::int_(src.width());
    size[1] = py::int_(src.height());

    auto image = frombytes(
        mode,
        size,
        py::bytes((const char*)src.bits(), src.sizeInBytes())
    );

    return image.release();
}

