#pragma once
#include "model/document_node.hpp"
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


namespace glaxnimate::model {

class ReferenceBase
{
    Q_GADGET

public:
    ReferenceBase(
        PropertyCallback<std::vector<DocumentNode*>> valid_options,
        PropertyCallback<bool, DocumentNode*> is_valid_option
    ) : valid_options_(std::move(valid_options)),
        is_valid_option_(std::move(is_valid_option))
    {}

    virtual ~ReferenceBase() {}

    virtual Object* object() const = 0;


    std::vector<DocumentNode*> valid_options() const
    {
        return valid_options_(object());
    }

    bool is_valid_option(DocumentNode* ptr) const
    {
        return is_valid_option_(object(), ptr);
    }

    virtual bool set_ref(model::DocumentNode* t) = 0;
    virtual model::DocumentNode* get_ref() const = 0;

    void transfer(Document* document);

    static ReferenceBase* cast(BaseProperty* property);

private:
    PropertyCallback<std::vector<DocumentNode*>> valid_options_;
    PropertyCallback<bool, DocumentNode*> is_valid_option_;

protected:
    static void remove_user(ReferenceBase* prop, model::DocumentNode* obj)
    {
        obj->remove_user(prop);
    }
    static void add_user(ReferenceBase* prop, model::DocumentNode* obj)
    {
        obj->add_user(prop);
    }
};

class ReferencePropertyBase : public BaseProperty, public ReferenceBase
{
public:
    ReferencePropertyBase(
        Object* obj,
        const QString& name,
        PropertyCallback<std::vector<DocumentNode*>> valid_options,
        PropertyCallback<bool, DocumentNode*> is_valid_option,
        PropertyTraits::Flags flags = PropertyTraits::Visual)
        : BaseProperty(obj, name, PropertyTraits{PropertyTraits::ObjectReference, flags}),
        ReferenceBase(std::move(valid_options), std::move(is_valid_option))
    {
    }

    Object* object() const override
    {
        return BaseProperty::object();
    }

    bool valid_value(const QVariant & v) const override
    {
        return this->is_valid_option(v.value<DocumentNode*>());
    }

    void transfer(Document* document) override
    {
        ReferenceBase::transfer(document);
    }

    void set_time(FrameTime) override {}

};

template<class Type>
class ReferenceProperty : public ReferencePropertyBase
{
public:
    using value_type = Type*;

    ReferenceProperty(
        Object* obj,
        const QString& name,
        PropertyCallback<std::vector<DocumentNode*>> valid_options,
        PropertyCallback<bool, DocumentNode*> is_valid_option,
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
        if ( old )
            remove_user(this, old);
        if ( value )
            add_user(this, value);
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
        if ( val.isNull() )
            return set(nullptr);
        if ( auto v = detail::variant_cast<Type*>(val) )
            return set(*v);
        return true;
    }

    Type* operator->() const
    {
        return value_;
    }

    bool set_ref(model::DocumentNode* t) override
    {
        if ( !t )
        {
            set_force(nullptr);
            return true;
        }
        if ( auto p = qobject_cast<Type*>(t) )
            return set(p);
        return false;
    }

    model::DocumentNode* get_ref() const override
    {
        return value_;
    }

private:
    Type* value_ = nullptr;
    PropertyCallback<void, Type*, Type*> on_changed;
};

} // namespace glaxnimate::model
