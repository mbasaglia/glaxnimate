#pragma once
#include "animatable.hpp"
#include "model/property/reference_property.hpp"

/**
 * GLAXNIMATE_ANIMATABLE_REFERENCE(type, name, valid_options, is_valid_option, [emitter, flags])
 */
#define GLAXNIMATE_ANIMATABLE_REFERENCE(type, name, ...)                                \
public:                                                                                 \
    glaxnimate::model::AnimatedReferenceProperty<type> name{this, #name, __VA_ARGS__};  \
    GLAXNIMATE_ANIMATABLE_IMPL(type, name)                                              \
    // macro end

namespace glaxnimate::model {

class AnimatableReferenceBase : public AnimatableBase, public ReferenceBase
{
public:
    AnimatableReferenceBase(
        Object* object,
        const QString& name,
        PropertyTraits traits,
        PropertyCallback<std::vector<DocumentNode*>> valid_options,
        PropertyCallback<bool, DocumentNode*> is_valid_option
    ) : AnimatableBase(object, name, traits),
        ReferenceBase(std::move(valid_options), std::move(is_valid_option))
    {}


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
};


template<class Type>
class AnimatedReferenceProperty : public detail::AnimatedProperty<Type*, AnimatableReferenceBase, UnlerpableKeyframe<Type*>>
{
    using Base = detail::AnimatedProperty<Type*, AnimatableReferenceBase, UnlerpableKeyframe<Type*>>;

public:
    AnimatedReferenceProperty(
        Object* obj,
        const QString& name,
        PropertyCallback<std::vector<DocumentNode*>> valid_options,
        PropertyCallback<bool, DocumentNode*> is_valid_option,
        PropertyCallback<void, Type*> emitter = {},
        int flags = 0
    ) : Base(
        obj, name, nullptr, std::move(emitter),
        PropertyTraits{PropertyTraits::ObjectReference, flags|PropertyTraits::Visual|PropertyTraits::Animated},
        std::move(valid_options), std::move(is_valid_option)
    )
    {
    }

    bool set_ref(model::DocumentNode* t) override
    {
        if ( !this->is_valid_option(t) )
            return false;
        return Base::set(static_cast<Type*>(t));
    }

    model::DocumentNode* get_ref() const override
    {
        return Base::get();
    }

    using AnimatableBase::set_keyframe;

protected:
    bool process_value(typename Base::value_type& value) const override
    {
        if ( !this->is_valid_option(value) )
        {
            value = nullptr;
            return false;
        }
        return true;
    }

};

} // namespace glaxnimate::model
