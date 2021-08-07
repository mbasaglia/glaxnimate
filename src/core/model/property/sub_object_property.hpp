#pragma once
#include "model/document_node.hpp"
#include "model/property/property.hpp"

#define GLAXNIMATE_SUBOBJECT_IMPL(type, name)               \
    type* get_##name() { return name.get(); }               \
private:                                                    \
    Q_PROPERTY(type* name READ get_##name)                  \

#define GLAXNIMATE_SUBOBJECT_EX(type, name, ...)            \
public:                                                     \
    SubObjectProperty<type> name{this, #name, __VA_ARGS__}; \
    GLAXNIMATE_SUBOBJECT_IMPL(type, name)                   \
    // macro end

#define GLAXNIMATE_SUBOBJECT(type, name)                    \
public:                                                     \
    SubObjectProperty<type> name{this, #name};              \
    GLAXNIMATE_SUBOBJECT_IMPL(type, name)                   \
    // macro end

namespace glaxnimate::model {

class SubObjectPropertyBase : public BaseProperty
{
public:
    SubObjectPropertyBase(Object* obj, const QString& name, int flags)
        : BaseProperty(obj, name, {PropertyTraits::Object, flags})
    {}

    virtual const model::Object* sub_object() const = 0;
    virtual model::Object* sub_object() = 0;

protected:
    void adjust_parent(Object*) const{}
    void adjust_parent(DocumentNode* ptr) const
    {
        ptr->added_to_list(static_cast<DocumentNode*>(object()));
    }
};

template<class Type>
class SubObjectProperty : public SubObjectPropertyBase
{
public:
    SubObjectProperty(Object* obj, const QString& name, int flags = 0)
        : SubObjectPropertyBase(obj, name, flags),
        sub_obj(obj->document())
    {
        adjust_parent(&sub_obj);
    }

    const Type* operator->() const
    {
        return &sub_obj;
    }

    Type* operator->()
    {
        return &sub_obj;
    }

    QVariant value() const override
    {
        return QVariant::fromValue(const_cast<Type*>(&sub_obj));
    }

    bool valid_value(const QVariant & v) const override
    {
        return v.value<Type*>();
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

    void transfer(Document* doc) override
    {
        sub_obj.transfer(doc);
    }

private:
    Type sub_obj;
};

} // namespace glaxnimate::model
