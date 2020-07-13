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
        return QVariant(value_);
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
    UnknownProperty(Object* obj, QString name, QVariant value)
        : BaseProperty(obj, std::move(name), std::move(name)),
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


} // namespace model
