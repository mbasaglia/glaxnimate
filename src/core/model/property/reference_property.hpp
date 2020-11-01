#pragma once
#include "model/reference_target.hpp"
#include "model/property/property.hpp"


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
    Q_CLASSINFO(#name, "property ref " #type)               \
    // macro end


namespace model {

class ReferencePropertyBase : public BaseProperty
{
    Q_GADGET
public:
    ReferencePropertyBase(
        Object* obj,
        const QString& name,
        PropertyCallback<std::vector<ReferenceTarget*>> valid_options,
        PropertyCallback<bool, ReferenceTarget*> is_valid_option,
        PropertyTraits::Flags flags = PropertyTraits::Visual)
        : BaseProperty(obj, name, PropertyTraits{PropertyTraits::ObjectReference, flags}),
        valid_options_(std::move(valid_options)),
        is_valid_option_(std::move(is_valid_option))
    {
    }

    bool valid_value(const QVariant & v) const override
    {
        return is_valid_option_(object(), v.value<ReferenceTarget*>());
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

    void transfer(Document*) override;

    virtual bool set_ref(model::ReferenceTarget* t) = 0;
    virtual model::ReferenceTarget* get_ref() const = 0;

private:
    PropertyCallback<std::vector<ReferenceTarget*>> valid_options_;
    PropertyCallback<bool, ReferenceTarget*> is_valid_option_;
};

template<class Type>
class ReferenceProperty : public ReferencePropertyBase
{
public:
    using value_type = Type*;

    ReferenceProperty(
        Object* obj,
        const QString& name,
        PropertyCallback<std::vector<ReferenceTarget*>> valid_options,
        PropertyCallback<bool, ReferenceTarget*> is_valid_option,
        PropertyCallback<void, Type*, Type*> on_changed = {},
        PropertyTraits::Flags flags = PropertyTraits::Visual)
        : ReferencePropertyBase(obj, name, std::move(valid_options), std::move(is_valid_option), flags),
        on_changed(std::move(on_changed))
    {
    }

    bool set(Type* value)
    {
        if ( !is_valid_option(value) )
            return false;
        set_force(value);
        return true;
    }

    void set_force(Type* value)
    {
        auto old = value_;
        value_ = value;
        value_changed();
        on_changed(object(), value_, old);
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

    Type* operator->() const
    {
        return value_;
    }

    bool set_ref(model::ReferenceTarget* t) override
    {
        if ( !t )
            return set(nullptr);
        if ( auto p = qobject_cast<Type*>(t) )
            return set(p);
        return false;
    }

    model::ReferenceTarget* get_ref() const override
    {
        return value_;
    }

private:
    Type* value_ = nullptr;
    PropertyCallback<void, Type*, Type*> on_changed;
};

} // namespace model
