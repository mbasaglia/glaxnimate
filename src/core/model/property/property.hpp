#pragma once

#include <type_traits>
#include <functional>
#include <iterator>
#include <optional>
#include <memory>

#include <QString>
#include <QPointF>
#include <QVector2D>
#include <QColor>
#include <QVariant>
#include <QGradient>

#include "model/animation/frame_time.hpp"

namespace math::bezier { class Bezier; }

namespace model {

class Object;
class Document;

struct PropertyTraits
{
    enum Type
    {
        Unknown,
        Object,
        ObjectReference,
        Bool,
        Int,
        Float,
        Point,
        Color,
        Size,
        Scale,
        String,
        Enum,
        Uuid,
        Bezier,
        Data,
        Gradient,
    };

    enum Flags
    {
        NoFlags = 0,
        List = 1,
        ReadOnly = 2,
        Animated = 4,
        Visual = 8,
    };


    Type type = Unknown;
    int flags = NoFlags;

    bool is_object() const
    {
        return type == Object || type == ObjectReference;
    }

    template<class T>
    static constexpr Type get_type() noexcept;

    template<class T>
    static PropertyTraits from_scalar(int flags=NoFlags)
    {
        return {
            get_type<T>(),
            flags
        };
    }
};


namespace detail {


template<class T, class = void>
struct GetType;

template<class ObjT>
static constexpr bool is_object_v = std::is_base_of_v<Object, ObjT> || std::is_same_v<Object, ObjT>;

// template<class ObjT>
// struct GetType<ObjT*, std::enable_if_t<is_object_v<ObjT>>>
// {
//     static constexpr const PropertyTraits::Type value = PropertyTraits::ObjectReference;
// };

template<class ObjT>
struct GetType<std::unique_ptr<ObjT>, std::enable_if_t<is_object_v<ObjT>>>
{
    static constexpr const PropertyTraits::Type value = PropertyTraits::Object;
};

template<> struct GetType<bool, void> { static constexpr const PropertyTraits::Type value = PropertyTraits::Bool; };
template<> struct GetType<float, void> { static constexpr const PropertyTraits::Type value = PropertyTraits::Float; };
template<> struct GetType<QVector2D, void> { static constexpr const PropertyTraits::Type value = PropertyTraits::Scale; };
template<> struct GetType<QColor, void> { static constexpr const PropertyTraits::Type value = PropertyTraits::Color; };
template<> struct GetType<QSizeF, void> { static constexpr const PropertyTraits::Type value = PropertyTraits::Size; };
template<> struct GetType<QString, void> { static constexpr const PropertyTraits::Type value = PropertyTraits::String; };
template<> struct GetType<QUuid, void> { static constexpr const PropertyTraits::Type value = PropertyTraits::Uuid; };
template<> struct GetType<QPointF, void> { static constexpr const PropertyTraits::Type value = PropertyTraits::Point; };
template<> struct GetType<math::bezier::Bezier, void> { static constexpr const PropertyTraits::Type value = PropertyTraits::Bezier; };
template<> struct GetType<QByteArray, void> { static constexpr const PropertyTraits::Type value = PropertyTraits::Data; };
template<> struct GetType<QGradientStops, void> { static constexpr const PropertyTraits::Type value = PropertyTraits::Gradient; };

template<class ObjT>
struct GetType<ObjT, std::enable_if_t<std::is_integral_v<ObjT>>>
{
    static constexpr const PropertyTraits::Type value = PropertyTraits::Int;
};
template<class ObjT>
struct GetType<ObjT, std::enable_if_t<std::is_enum_v<ObjT>>>
{
    static constexpr const PropertyTraits::Type value = PropertyTraits::Enum;
};
} // namespace detail


template<class T>
inline constexpr PropertyTraits::Type PropertyTraits::get_type() noexcept
{
    return detail::GetType<T>::value;
}


#define GLAXNIMATE_PROPERTY(type, name, ...)                \
public:                                                     \
    Property<type> name{this, #name, __VA_ARGS__};          \
    type get_##name() const { return name.get(); }          \
    bool set_##name(const type& v) {                        \
        return name.set_undoable(QVariant::fromValue(v));   \
    }                                                       \
private:                                                    \
    Q_PROPERTY(type name READ get_##name WRITE set_##name)  \
    Q_CLASSINFO(#name, "property " #type)                   \
    // macro end

#define GLAXNIMATE_PROPERTY_RO(type, name, default_value)   \
public:                                                     \
    Property<type> name{this, #name, default_value, {}, {}, PropertyTraits::ReadOnly}; \
    type get_##name() const { return name.get(); }          \
private:                                                    \
    Q_PROPERTY(type name READ get_##name)                   \
    Q_CLASSINFO(#name, "property " #type)                   \
    // macro end

class BaseProperty
{
public:
    BaseProperty(Object* object, const QString& name, PropertyTraits traits);

    virtual ~BaseProperty() = default;

    virtual QVariant value() const = 0;
    virtual bool set_value(const QVariant& val) = 0;
    virtual bool set_undoable(const QVariant& val, bool commit = true);
    virtual void set_time(FrameTime t) = 0;
    virtual void transfer(Document*) {};
    virtual bool valid_value(const QVariant& v) const = 0;

    virtual bool assign_from(const BaseProperty* prop)
    {
        return set_value(prop->value());
    }

    const QString& name() const
    {
        return name_;
    }

    PropertyTraits traits() const
    {
        return traits_;
    }

    Object* object() const
    {
        return object_;
    }

protected:
    void value_changed();

private:
    Object* object_;
    QString name_;
    PropertyTraits traits_;
};

namespace detail {

template<class T> inline T defval() { return T(); }
template<> inline void defval<void>() {}


template<class FuncT, class... Args, std::size_t... I>
auto invoke_impl(const FuncT& fun, std::index_sequence<I...>, const std::tuple<Args...>& args)
{
  return fun(std::get<I>(args)...);
}

template<int ArgCount, class FuncT, class... Args>
auto invoke(const FuncT& fun, const Args&... t)
{
  return invoke_impl(fun, std::make_index_sequence<ArgCount>(), std::make_tuple(t...));
}

} // namespace detail

template<class Return, class... ArgType>
class PropertyCallback
{
private:
    class HolderBase
    {
    public:
        virtual ~HolderBase() = default;
        virtual Return invoke(Object* obj, const ArgType&... v) const = 0;
    };

    template<class ObjT, class... Arg>
    class Holder : public HolderBase
    {
    public:
        using FuncP = std::function<Return (ObjT*, Arg...)>;

        Holder(FuncP func) : func(std::move(func)) {}

        Return invoke(Object* obj, const ArgType&... v) const override
        {
            return detail::invoke<sizeof...(Arg)+1>(func, static_cast<ObjT*>(obj), v...);
        }

        FuncP func;
    };
    std::unique_ptr<HolderBase> holder;
public:
    PropertyCallback() = default;

    PropertyCallback(std::nullptr_t) {}

    template<class ObjT, class... Arg>
    PropertyCallback(Return (ObjT::*func)(Arg...)) : holder(std::make_unique<Holder<ObjT, Arg...>>(func)) {}
    template<class ObjT, class... Arg>
    PropertyCallback(Return (ObjT::*func)(Arg...) const) : holder(std::make_unique<Holder<ObjT, Arg...>>(func)) {}


    explicit operator bool() const
    {
        return bool(holder);
    }

    Return operator() (Object* obj, const ArgType&... v) const
    {
        if ( holder )
            return holder->invoke(obj, v...);
        return detail::defval<Return>();
    }
};


namespace detail {

template<class Type>
std::optional<Type> variant_cast(const QVariant& val)
{
    if ( !val.canConvert(qMetaTypeId<Type>()) )
        return {};
    QVariant converted = val;
    if ( !converted.convert(qMetaTypeId<Type>()) )
        return {};
    return converted.value<Type>();
}

} // namespace detail

template<class Type, class Reference = const Type&>
class Property : public BaseProperty
{
public:
    using value_type = Type;
    using reference = Reference;

    Property(Object* obj,
             const QString& name,
             Type default_value = Type(),
             PropertyCallback<void, Type> emitter = {},
             PropertyCallback<bool, Type> validator = {},
             PropertyTraits::Flags flags = PropertyTraits::NoFlags
    )
        : BaseProperty(obj, name, PropertyTraits::from_scalar<Type>(flags)),
          value_(std::move(default_value)),
          emitter(std::move(emitter)),
          validator(std::move(validator))
    {}

    bool valid_value(const QVariant& val) const override
    {
        if ( auto v = detail::variant_cast<Type>(val) )
            return !validator || validator(object(), *v);
        return false;
    }

    bool set(Type value)
    {
        if ( validator && !validator(object(), value) )
            return false;
        std::swap(value_, value);
        value_changed();
        if ( emitter )
            emitter(object(), value_);
        return true;
    }

    reference get() const
    {
        return value_;
    }

    QVariant value() const override
    {
        return QVariant::fromValue(value_);
    }

    bool set_value(const QVariant& val) override
    {
        if ( auto v = detail::variant_cast<Type>(val) )
            return set(*v);
        return false;
    }

    void set_time(FrameTime) override {}

private:
    Type value_;
    PropertyCallback<void, Type> emitter;
    PropertyCallback<bool, Type> validator;
};

} // namespace model
