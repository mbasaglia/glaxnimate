#pragma once

#include <QString>

#include "object.hpp"

namespace model {


class BaseProperty
{
public:
    BaseProperty(Object* obj, QString name, QString lottie)
        : obj(std::move(obj)), name_(std::move(name)), lottie_(lottie)
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

    const QString& lottie() const
    {
        return lottie_;
    }

protected:
    void value_changed()
    {
        obj->property_value_changed(name_, value());
    }

private:
    Object* obj;
    QString name_;
    QString lottie_;
};


template<class Type, class Reference = const Type&>
class FixedValueProperty : public BaseProperty
{
public:
    using held_type = Type;
    using reference = Reference;

    FixedValueProperty(Object* obj, QString name, QString lottie, Type value)
        : BaseProperty(obj, std::move(name), std::move(lottie)),
          value_(std::move(value))
    {}

    reference get() const
    {
        return value_;
    }

    QVariant value() const override
    {
        return QVariant::fromValue(value_);
    }

    bool set_value(const QVariant&) override
    {
        return false;
    }

private:
    Type value_;
};


template<class Type, class Reference = const Type&>
class Property : public BaseProperty
{
public:
    using held_type = Type;
    using reference = Reference;

    Property(Object* obj, QString name, QString lottie, Type default_value = Type())
        : BaseProperty(obj, std::move(name), std::move(lottie)),
          value_(std::move(default_value))
    {}

    void set(Type&& default_value)
    {
        std::swap(value_, default_value);
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
        if ( !val.canConvert<Type>() )
            return false;
        set(val.value<Type>());
        return true;
    }

private:
    Type value_;
};


class UnknownProperty : public BaseProperty
{
public:
    UnknownProperty(Object* obj, const QString& name, QVariant value)
        : BaseProperty(obj, name, name),
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
    using const_reference = const Type&;
    using iterator = typename std::vector<pointer>::iterator;
    using const_iterator = typename std::vector<pointer>::const_iterator;

    ObjectListProperty(Object* obj, QString name, QString lottie)
        : BaseProperty(obj, std::move(name), std::move(lottie))
    {}

    reference operator[](int i) { return *objects[i]; }
    const_reference operator[](int i) const { return *objects[i]; }
    int size() const { return objects.size(); }
    bool empty() const { return objects.empty(); }
    iterator begin() { return objects.begin(); }
    iterator end() { return objects.end(); }
    const_iterator begin() const { return objects.begin(); }
    const_iterator end() const { return objects.end(); }

    void push_back(pointer p)
    {
        objects.push_back(std::move(p));
    }

    bool valid_index(int index)
    {
        return index < 0 || index >= objects.size();
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
    }

private:
    std::vector<pointer> objects;

};


} // namespace model
