#pragma once

#include <type_traits>
#include <functional>

#include <QString>
#include <QPointF>
#include <QVector2D>
#include <QColor>

#include "object.hpp"

namespace math { class Bezier; }

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
        Bezier,
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
template<> struct GetType<math::Bezier, void> { static constexpr const PropertyTraits::Type value = PropertyTraits::Bezier; };

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

/**
 * \brief Sets up declarations of concrete Object sub-classes
 * \note Call GLAXNIMATE_OBJECT_IMPL for every class declared with GLAXNIMATE_OBJECT
 */
#define GLAXNIMATE_OBJECT   \
    Q_OBJECT                \
    static bool _reg;

/**
 * \brief Registers a class declared with GLAXNIMATE_OBJECT to be constructed
 * with model::Factory
 */
#define GLAXNIMATE_OBJECT_IMPL(cls) \
    bool cls::_reg{model::Factory::instance().register_type<cls>()};

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

#define GLAXNIMATE_PROPERTY_REFERENCE(type, name, ...)      \
public:                                                     \
    ReferenceProperty<type> name{this, #name, __VA_ARGS__}; \
    type* get_##name() const { return name.get(); }         \
    bool set_##name(type* v)                                \
    {                                                       \
        return name.set_undoable(QVariant::fromValue(v));   \
    }                                                       \
private:                                                    \
    Q_PROPERTY(type* name READ get_##name WRITE set_##name) \
    // macro end

#define GLAXNIMATE_PROPERTY_LIST_IMPL(name)                 \
public:                                                     \
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

#define GLAXNIMATE_PROPERTY_LIST(type, name, ...)           \
public:                                                     \
    ObjectListProperty<type> name{this, #name, __VA_ARGS__};\
    GLAXNIMATE_PROPERTY_LIST_IMPL(name)                     \
    // macro end


#define GLAXNIMATE_PROPERTY_RO(type, name, default_value)   \
public:                                                     \
    Property<type> name{this, #name, default_value, {}, {}, PropertyTraits::ReadOnly}; \
    type get_##name() const { return name.get(); }          \
private:                                                    \
    Q_PROPERTY(type name READ get_##name)                   \
    // macro end


#define GLAXNIMATE_ANIMATABLE(type, name, default_value)        \
public:                                                         \
    AnimatedProperty<type> name{this, #name, default_value};    \
    AnimatableBase* get_##name() { return &name; }              \
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
    virtual void set_time(FrameTime t) = 0;

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
    void value_changed()
    {
        object_->property_value_changed(this, value());
    }

private:
    Object* object_;
    QString name_;
    PropertyTraits traits_;
};


namespace detail {

template<class T> inline T defval() { return T(); }
template<> inline void defval<void>() {}

} // namespace detail

template<class Return, class ArgType>
class PropertyCallback
{
private:
    class HolderBase
    {
    public:
        virtual ~HolderBase() = default;
        virtual Return invoke(Object* obj, const ArgType& v) const = 0;
    };

    template<class ObjT, class Arg>
    class Holder : public HolderBase
    {
    public:
        using FuncP = std::function<Return (ObjT*, Arg)>;

        Holder(FuncP func) : func(std::move(func)) {}

        Return invoke(Object* obj, const ArgType& v) const override
        {
            return func(static_cast<ObjT*>(obj), v);
        }

        FuncP func;
    };

    template<class ObjT, class Arg>
    class HolderConst : public HolderBase
    {
    public:
        using FuncP = std::function<Return (const ObjT*, Arg)>;

        HolderConst(FuncP func) : func(std::move(func)) {}

        Return invoke(Object* obj, const ArgType& v) const override
        {
            return func(static_cast<ObjT*>(obj), v);
        }

        FuncP func;
    };

    template<class ObjT>
    class HolderNoarg : public HolderBase
    {
    public:
        using FuncP = std::function<Return (ObjT*)>;

        HolderNoarg(FuncP func) : func(std::move(func)) {}

        Return invoke(Object* obj, const ArgType&) const override
        {
            return func(static_cast<ObjT*>(obj));
        }

        FuncP func;
    };

    std::unique_ptr<HolderBase> holder;
public:
    PropertyCallback() = default;

    PropertyCallback(std::nullptr_t) {}

    template<class ObjT, class Arg>
    PropertyCallback(Return (ObjT::*func)(const Arg&)) : holder(std::make_unique<Holder<ObjT, const Arg&>>(func)) {}
    template<class ObjT, class Arg>
    PropertyCallback(Return (ObjT::*func)(const Arg&) const) : holder(std::make_unique<HolderConst<ObjT, const Arg&>>(func)) {}
    template<class ObjT, class Arg>
    PropertyCallback(Return (ObjT::*func)(Arg)) : holder(std::make_unique<Holder<ObjT, Arg>>(func)) {}
    template<class ObjT, class Arg>
    PropertyCallback(Return (ObjT::*func)(Arg) const) : holder(std::make_unique<HolderConst<ObjT, Arg>>(func)) {}
    template<class ObjT>
    PropertyCallback(Return (ObjT::*func)()) : holder(std::make_unique<HolderNoarg<ObjT>>(func)) {}
    template<class ObjT>
    PropertyCallback(Return (ObjT::*func)() const) : holder(std::make_unique<HolderNoarg<ObjT>>(func)) {}


    explicit operator bool() const
    {
        return bool(holder);
    }

    Return operator() (Object* obj, const ArgType& v) const
    {
        if ( holder )
            return holder->invoke(obj, v);
        return detail::defval<Return>();
    }
};


template<class Return>
class PropertyCallback<Return, void>
{
private:

    class HolderBase
    {
    public:
        virtual ~HolderBase() = default;
        virtual Return invoke(Object* obj) const = 0;
    };

    template<class ObjT>
    class Holder : public HolderBase
    {
    public:
        using FuncP = std::function<Return (ObjT*)>;

        Holder(FuncP func) : func(std::move(func)) {}

        Return invoke(Object* obj) const override
        {
            return func(static_cast<ObjT*>(obj));
        }

        FuncP func;
    };

    template<class ObjT>
    class HolderConst : public HolderBase
    {
    public:
        using FuncP = std::function<Return (const ObjT*)>;

        HolderConst(FuncP func) : func(std::move(func)) {}

        Return invoke(Object* obj) const override
        {
            return func(static_cast<ObjT*>(obj));
        }

        FuncP func;
    };

    std::unique_ptr<HolderBase> holder;
public:
    PropertyCallback() = default;

    PropertyCallback(std::nullptr_t) {}

    template<class ObjT>
    PropertyCallback(Return (ObjT::*func)()) : holder(std::make_unique<Holder<ObjT>>(func)) {}
    template<class ObjT>
    PropertyCallback(Return (ObjT::*func)() const) : holder(std::make_unique<HolderConst<ObjT>>(func)) {}

    explicit operator bool() const
    {
        return bool(holder);
    }

    Return operator() (Object* obj) const
    {
        if ( holder )
            return holder->invoke(obj);
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

    void set_time(FrameTime) override {}

private:
    QVariant variant;
};


class ObjectListPropertyBase : public BaseProperty
{
public:
    ObjectListPropertyBase(Object* obj, const QString& name)
        : BaseProperty(obj, name, {PropertyTraits::Object, PropertyTraits::List|PropertyTraits::Visual})
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

    ObjectListProperty(
        Object* obj,
        const QString& name,
        PropertyCallback<void, Type*> callback_insert = {},
        PropertyCallback<void, Type*> callback_remove = {},
        PropertyCallback<void, int> callback_insert_begin = {},
        PropertyCallback<void, int> callback_remove_begin = {}
    )
        : ObjectListPropertyBase(obj, name),
        callback_insert(std::move(callback_insert)),
        callback_remove(std::move(callback_remove)),
        callback_insert_begin(std::move(callback_insert_begin)),
        callback_remove_begin(std::move(callback_remove_begin))
    {}

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

        callback_insert_begin(this->object(), position);
        auto ptr = p.get();
        objects.insert(objects.begin()+position, std::move(p));
        on_insert(position);
        callback_insert(this->object(), ptr);
    }

    bool valid_index(int index)
    {
        return index >= 0 && index < int(objects.size());
    }

    pointer remove(int index)
    {
        if ( !valid_index(index) )
            return {};
        callback_remove_begin(object(), index);
        auto it = objects.begin() + index;
        auto v = std::move(*it);
        objects.erase(it);
        on_remove(index);
        callback_remove(object(), v.get());
        return v;
    }

    void swap(int index_a, int index_b)
    {
        if ( !valid_index(index_a) || !valid_index(index_b) || index_a == index_b )
            return;

        std::swap(objects[index_a], objects[index_b]);
        on_swap(index_a, index_b);
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

    void set_time(FrameTime t) override
    {
        for ( const auto& o : objects )
            o->set_time(t);
    }

    int index_of(value_type* obj, int not_found = -1) const
    {
        for ( int i = 0; i < size(); i++ )
            if ( objects[i].get() == obj )
                return i;
        return not_found;
    }

protected:
    virtual void on_insert(int index) { Q_UNUSED(index); }
    virtual void on_remove(int index) { Q_UNUSED(index); }
    virtual void on_swap(int index_a, int index_b) { Q_UNUSED(index_a); Q_UNUSED(index_b); }

    std::vector<pointer> objects;
    PropertyCallback<void, Type*> callback_insert;
    PropertyCallback<void, Type*> callback_remove;
    PropertyCallback<void, int> callback_insert_begin;
    PropertyCallback<void, int> callback_remove_begin;
};


class SubObjectPropertyBase : public BaseProperty
{
public:
    SubObjectPropertyBase(Object* obj, const QString& name)
        : BaseProperty(obj, name, {PropertyTraits::Object})
    {}

    virtual const model::Object* sub_object() const = 0;
    virtual model::Object* sub_object() = 0;
};

template<class Type>
class SubObjectProperty : public SubObjectPropertyBase
{
public:
    SubObjectProperty(Object* obj, const QString& name)
        : SubObjectPropertyBase(obj, name),
        sub_obj(obj->document())
    {}

    QVariant value() const override
    {
        return QVariant::fromValue(const_cast<Type*>(&sub_obj));
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

        sub_obj.assign_from(object);
        return &sub_obj;
    }

    Type* get() { return &sub_obj; }
    const Type* get() const { return &sub_obj; }

    model::Object * sub_object() override { return &sub_obj; }
    const model::Object * sub_object() const override { return &sub_obj; }


    void set_time(FrameTime t) override
    {
        sub_obj.set_time(t);
    }

private:
    Type sub_obj;
};

} // namespace model
