#pragma once

#include <QString>

#include <type_traits>

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
        String,
        Enum,
        Uuid,
    };
    bool list = false;
    Type type = Unknown;
    bool user_editable = true;

    bool is_object() const
    {
        return type == Object || type == ObjectReference;
    }

    template<class T>
    static constexpr Type get_type() noexcept;

    template<class T>
    static PropertyTraits from_scalar(bool list=false, bool user_editable=true)
    {
        return {
            list,
            get_type<T>(),
            user_editable
        };
    }
};


namespace detail {


template<class T, class = void>
struct GetType;

template<class ObjT>
static constexpr bool is_object_v = std::is_base_of_v<Object, ObjT> || std::is_same_v<Object, ObjT>;

template<class ObjT>
struct GetType<ObjT*, std::enable_if_t<is_object_v<ObjT>>>
{
    static constexpr const PropertyTraits::Type value = PropertyTraits::ObjectReference;
};

template<class ObjT>
struct GetType<std::unique_ptr<ObjT>, std::enable_if_t<is_object_v<ObjT>>>
{
    static constexpr const PropertyTraits::Type value = PropertyTraits::Object;
};

template<> struct GetType<bool, void> { static constexpr const PropertyTraits::Type value = PropertyTraits::Bool; };
template<> struct GetType<float, void> { static constexpr const PropertyTraits::Type value = PropertyTraits::Float; };
template<> struct GetType<QVector2D, void> { static constexpr const PropertyTraits::Type value = PropertyTraits::Point; };
template<> struct GetType<QColor, void> { static constexpr const PropertyTraits::Type value = PropertyTraits::Color; };
template<> struct GetType<QSizeF, void> { static constexpr const PropertyTraits::Type value = PropertyTraits::Size; };
template<> struct GetType<QString, void> { static constexpr const PropertyTraits::Type value = PropertyTraits::String; };
template<> struct GetType<QUuid, void> { static constexpr const PropertyTraits::Type value = PropertyTraits::Uuid; };

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


class BaseProperty
{
public:
    BaseProperty(Object* obj, QString name, PropertyTraits traits)
        : obj(std::move(obj)), name_(std::move(name)), traits_(traits)
    {
        obj->add_property(this);
    }

    virtual ~BaseProperty() = default;

    virtual QVariant value() const = 0;
    virtual bool set_value(const QVariant& val) = 0;


    const QString& name() const
    {
        return name_;
    }

    PropertyTraits traits() const
    {
        return traits_;
    }

protected:
    void value_changed()
    {
        obj->property_value_changed(name_, value());
    }

private:
    Object* obj;
    QString name_;
    PropertyTraits traits_;
};


template<class Type, class Reference = const Type&>
class Property : public BaseProperty
{
public:
    using held_type = Type;
    using reference = Reference;

    Property(Object* obj, QString name, Type default_value = Type(), bool user_editable=true)
        : BaseProperty(obj, std::move(name), PropertyTraits::from_scalar<Type>(false, user_editable)),
          value_(std::move(default_value))
    {}

    void set(Type value)
    {
        std::swap(value_, value);
        value_changed();
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
        if ( !val.canConvert(qMetaTypeId<Type>()) )
            return false;
        QVariant converted = val;
        if ( !converted.convert(qMetaTypeId<Type>()) )
            return false;
        set(converted.value<Type>());
        return true;
    }

private:
    Type value_;
};


class UnknownProperty : public BaseProperty
{
public:
    UnknownProperty(Object* obj, const QString& name, QVariant value)
        : BaseProperty(obj, name, {false, PropertyTraits::Unknown, false}),
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


template<class Type>
class ObjectListProperty : public BaseProperty
{
public:
    using held_type = Type;
    using pointer = std::unique_ptr<Type>;
    using reference = Type&;
//     using const_reference = const Type&;
    using iterator = typename std::vector<pointer>::const_iterator;
//     using const_iterator = typename std::vector<pointer>::const_iterator;

    ObjectListProperty(Object* obj, QString name)
        : BaseProperty(obj, std::move(name), {true, PropertyTraits::Object})
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

    void insert(pointer p, int position)
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

    void insert(int index, pointer p)
    {
        if ( index + 1 >= objects.size() )
        {
            objects.push_back(std::move(p));
            return;
        }

        if ( index <= 0 )
            index = 0;

        objects.insert(objects.begin() + index, std::move(p));
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

    bool set_value(const QVariant& val) override
    {
        if ( !val.canConvert<QVariantList>() )
            return false;

        for ( const auto& v : val.toList() )
        {
            if ( !v.canConvert<Object*>() )
                continue;

            if ( Object* obj = v.value<Object*>() )
            {
                auto basep = obj->clone();
                Type* casted = qobject_cast<Type*>(basep.get());
                if ( casted )
                {
                    basep.release();
                    objects.push_back(pointer(casted));
                }
            }
        }

        return true;
    }

private:
    std::vector<pointer> objects;

};

} // namespace model
