#pragma once
#include "model/reference_target.hpp"
#include "model/property/property.hpp"

namespace model {

class ReferencePropertyBase : public BaseProperty
{
    Q_GADGET
public:
    ReferencePropertyBase(
        Object* obj,
        const QString& name,
        PropertyCallback<std::vector<ReferenceTarget*>, void> valid_options,
        PropertyCallback<bool, ReferenceTarget*> is_valid_option,
        PropertyTraits::Flags flags = PropertyTraits::Visual)
        : BaseProperty(obj, name, PropertyTraits{PropertyTraits::ObjectReference, flags}),
        valid_options_(std::move(valid_options)),
        is_valid_option_(std::move(is_valid_option))
    {
    }

    std::vector<ReferenceTarget*> valid_options() const
    {
        return valid_options_(object());
    }

    bool is_valid_option(ReferenceTarget* ptr) const
    {
        return is_valid_option_(object(), ptr);
    }

    void set_time(FrameTime) override {}

private:
    PropertyCallback<std::vector<ReferenceTarget*>, void> valid_options_;
    PropertyCallback<bool, ReferenceTarget*> is_valid_option_;
};

template<class Type>
class ReferenceProperty : public ReferencePropertyBase
{
public:
    using value_type = Type*;

    using ReferencePropertyBase::ReferencePropertyBase;

    bool set(Type* value)
    {
        if ( !is_valid_option(value) )
            return false;
        value_ = value;
        value_changed();
        return true;
    }

    Type* get() const
    {
        return value_;
    }

    QVariant value() const override
    {
        if ( !value_ )
            return {};
        return QVariant::fromValue(value_);
    }

    bool set_value(const QVariant& val) override
    {
        if ( auto v = detail::variant_cast<Type*>(val) )
            return set(*v);
        return true;
    }

private:
    Type* value_ = nullptr;
};

} // namespace model
