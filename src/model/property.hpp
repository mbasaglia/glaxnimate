#pragma once

#include <type_traits>

#include <QString>
#include <QPointF>
#include <QVector2D>

#include "object.hpp"

namespace model {

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
    };

    enum Flags
    {
        NoFlags = 0,
        List = 1,
        ReadOnly = 2,
        Animated = 4,
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

    app::settings::Setting::Type setting_type() const
    {
        switch ( type )
        {
            case Bool:
                return app::settings::Setting::Bool;
            case Int:
                return app::settings::Setting::Int;
            case Float:
                return app::settings::Setting::Float;
            case String:
                return app::settings::Setting::String;
            case Color:
                return app::settings::Setting::Color;
            default:
                return app::settings::Setting::Internal;
        }
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
    // macro end

#define GLAXNIMATE_PROPERTY_REFERENCE(type, name)           \
public:                                                     \
    ReferenceProperty<type> name{this, #name};              \
    type* get_##name() const { return name.get(); }         \
    bool set_##name(type* v)                                \
    {                                                       \
        return name.set_undoable(QVariant::fromValue(v));   \
    }                                                       \
private:                                                    \
    Q_PROPERTY(type* name READ get_##name WRITE set_##name) \
    // macro end

#define GLAXNIMATE_PROPERTY_LIST(type, name)                \
public:                                                     \
    ObjectListProperty<type> name{this, #name};             \
    QVariantList get_##name() const                         \
    {                                                       \
        QVariantList ret;                                   \
        for ( const auto & ptr : name )                     \
            ret.push_back(QVariant::fromValue(ptr.get()));  \
        return ret;                                         \
    }                                                       \
private:                                                    \
    Q_PROPERTY(QVariantList name READ get_##name)           \
    // macro end

#define GLAXNIMATE_PROPERTY_RO(type, name, default_value)   \
public:                                                     \
    Property<type> name{this, #name, default_value, false}; \
    type get_##name() const { return name.get(); }          \
private:                                                    \
    Q_PROPERTY(type name READ get_##name)                   \
    // macro end


#define GLAXNIMATE_ANIMATABLE(type, name, default_value)        \
public:                                                         \
    AnimatedProperty<type> name{this, #name, default_value};    \
    AnimatableBase* get_##name() { return &name.animatable(); } \
private:                                                        \
    Q_PROPERTY(AnimatableBase* name READ get_##name)            \
    // macro end

#define GLAXNIMATE_SUBOBJECT(type, name)                    \
public:                                                     \
    SubObjectProperty<type> name{this, #name};              \
    type* get_##name() { return name.get(); }               \
private:                                                    \
    Q_PROPERTY(type* name READ get_##name)                  \
    // macro end


class BaseProperty
{
public:
    BaseProperty(Object* object, const QString& name, PropertyTraits traits)
        : object_(object), name_(name), traits_(traits)
    {
        object_->add_property(this);
    }

    virtual ~BaseProperty() = default;

    virtual QVariant value() const = 0;
    virtual bool set_value(const QVariant& val) = 0;
    virtual bool set_undoable(const QVariant& val);
    virtual void add_setting(app::settings::SettingList& list) const {
        auto setting_type = traits_.setting_type();
        if ( setting_type != app::settings::Setting::Internal )
            list.emplace_back(name_, name_, "", setting_type, value());
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
    void value_changed()
    {
        object_->property_value_changed(name_, value());
    }

private:
    Object* object_;
    QString name_;
    PropertyTraits traits_;
};


template<class Return, class Type>
class PropertyCallback
{
private:
    class HolderBase
    {
    public:
        virtual ~HolderBase() = default;
        virtual Return invoke(Object* obj, const Type& v) const = 0;
    };

    template<class ObjT>
    class HolderRef : public HolderBase
    {
    public:
        using FuncP = Return (ObjT::*)(const Type&);

        HolderRef(FuncP func) : func(func) {}

        Return invoke(Object* obj, const Type& v) const override
        {
            return (static_cast<ObjT*>(obj)->*func)(v);
        }

        FuncP func;
    };

    template<class ObjT>
    class HolderConstRef : public HolderBase
    {
    public:
        using FuncP = Return (ObjT::*)(const Type&) const;

        HolderConstRef(FuncP func) : func(func) {}

        Return invoke(Object* obj, const Type& v) const override
        {
            return (static_cast<const ObjT*>(obj)->*func)(v);
        }

        FuncP func;
    };

    template<class ObjT>
    class HolderVal : public HolderBase
    {
    public:
        using FuncP = Return (ObjT::*)(Type);

        HolderVal(FuncP func) : func(func) {}

        Return invoke(Object* obj, const Type& v) const override
        {
            return (static_cast<ObjT*>(obj)->*func)(v);
        }

        FuncP func;
    };

    template<class ObjT>
    class HolderConstVal : public HolderBase
    {
    public:
        using FuncP = Return (ObjT::*)(Type) const;

        HolderConstVal(FuncP func) : func(func) {}

        Return invoke(Object* obj, const Type& v) const override
        {
            return (static_cast<const ObjT*>(obj)->*func)(v);
        }

        FuncP func;
    };

    std::unique_ptr<HolderBase> holder;

public:
    PropertyCallback() = default;

    PropertyCallback(std::nullptr_t) {}

    template<class T>
    PropertyCallback(Return (T::*func)(const Type&)) : holder(std::make_unique<HolderRef<T>>(func)) {}

    template<class T>
    PropertyCallback(Return (T::*func)(const Type&) const) : holder(std::make_unique<HolderConstRef<T>>(func)) {}

    template<class T>
    PropertyCallback(Return (T::*func)(Type)) : holder(std::make_unique<HolderVal<T>>(func)) {}

    template<class T>
    PropertyCallback(Return (T::*func)(Type) const) : holder(std::make_unique<HolderConstVal<T>>(func)) {}

    explicit operator bool() const
    {
        return bool(holder);
    }

    Return operator() (Object* obj, const Type& v)
    {
        return holder->invoke(obj, v);
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
             bool user_editable=true,
             PropertyCallback<void, Type> emitter = {},
             PropertyCallback<bool, Type> validator = {}
    )
        : BaseProperty(obj, name, PropertyTraits::from_scalar<Type>(user_editable ? PropertyTraits::NoFlags : PropertyTraits::ReadOnly)),
          value_(std::move(default_value)),
          emitter(std::move(emitter)),
          validator(std::move(validator))
    {}

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

private:
    Type value_;
    PropertyCallback<void, Type> emitter;
    PropertyCallback<bool, Type> validator;
};


class UnknownProperty : public BaseProperty
{
public:
    UnknownProperty(Object* obj, const QString& name, QVariant value)
        : BaseProperty(obj, name, {PropertyTraits::Unknown, PropertyTraits::ReadOnly}),
          variant(std::move(value))
    {}

    QVariant value() const override
    {
        return variant;
    }

    bool set_value(const QVariant& val) override
    {
        variant = val;
        value_changed();
        return true;
    }

private:
    QVariant variant;
};


class ObjectListPropertyBase : public BaseProperty
{
public:
    ObjectListPropertyBase(Object* obj, const QString& name)
        : BaseProperty(obj, name, {PropertyTraits::Object, PropertyTraits::List})
    {}

    /**
     * \brief Inserts a clone of the passed object
     * \return The internal object or \b nullptr in case of failure
     */
    virtual Object* insert_clone(Object* object, int index = -1) = 0;


    bool set_value(const QVariant& val) override
    {
        if ( !val.canConvert<QVariantList>() )
            return false;

        for ( const auto& v : val.toList() )
        {
            if ( !v.canConvert<Object*>() )
                continue;

            insert_clone(v.value<Object*>());
        }

        return true;
    }
};


template<class Type>
class ObjectListProperty : public ObjectListPropertyBase
{
public:
    using value_type = Type;
    using pointer = std::unique_ptr<Type>;
    using reference = Type&;
//     using const_reference = const Type&;
    using iterator = typename std::vector<pointer>::const_iterator;
//     using const_iterator = typename std::vector<pointer>::const_iterator;

    using ObjectListPropertyBase::ObjectListPropertyBase;

    reference operator[](int i) const { return *objects[i]; }
    int size() const { return objects.size(); }
    bool empty() const { return objects.empty(); }
    iterator begin() const { return objects.begin(); }
    iterator end() const { return objects.end(); }

    reference back() const
    {
        return *objects.back();
    }

    void insert(pointer p, int position = -1)
    {
        if ( !valid_index(position) )
            position = size();
        objects.insert(objects.begin()+position, std::move(p));
    }

    bool valid_index(int index)
    {
        return index >= 0 && index < int(objects.size());
    }

    pointer remove(int index)
    {
        if ( !valid_index(index) )
            return {};
        auto it = objects.begin() + index;
        auto v = std::move(*it);
        objects.erase(it);
        return v;
    }

    void swap(int index_a, int index_b)
    {
        if ( !valid_index(index_a) || !valid_index(index_b) || index_a == index_b )
            return;

        std::swap(objects[index_a], objects[index_b]);
    }

    QVariant value() const override
    {
        QVariantList list;
        for ( const auto& p : objects )
            list.append(QVariant::fromValue((Object*)p.get()));
        return list;
    }

    Object* insert_clone(Object* object, int index = -1) override
    {
        if ( !object )
            return nullptr;

        auto basep = object->clone();

        Type* casted = qobject_cast<Type*>(basep.get());
        if ( casted )
        {
            basep.release();
            insert(pointer(casted), index);
            return casted;
        }
        return nullptr;
    }

private:
    std::vector<pointer> objects;
};


template<class Type>
class SubObjectProperty : public BaseProperty
{
public:
    SubObjectProperty(Object* obj, const QString& name)
        : BaseProperty(obj, name, {PropertyTraits::Object}),
        sub_obj(std::make_unique<Type>(obj->document()))
    {}

    QVariant value() const override
    {
        return QVariant::fromValue(const_cast<Type*>(get()));
    }

    bool set_value(const QVariant& val) override
    {
        if ( !val.canConvert<Type*>() )
            return false;

        if ( Type* t = val.value<Type*>() )
            return set_clone(t);

        return false;
    }

    Type* set_clone(Type* object)
    {
        if ( !object )
            return nullptr;

        sub_obj = object->clone_covariant();
        return sub_obj.get();
    }

    Type* get() { return sub_obj.get(); }
    const Type* get() const { return sub_obj.get(); }

private:
    std::unique_ptr<Type> sub_obj;
};

} // namespace model
