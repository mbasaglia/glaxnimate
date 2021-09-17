#pragma once

#include "property.hpp"


#define GLAXNIMATE_PROPERTY_OPTIONS(type, name, defval, container, ...)         \
public:                                                                         \
    OptionListProperty<type, container> name{this, #name, defval, __VA_ARGS__}; \
    GLAXNIMATE_PROPERTY_IMPL(type, name)                                        \
    Q_PROPERTY(QVariantList name##_options READ name##_options)                 \
    QVariantList name##_options() const { return name.value_options(); }        \
    // macro end


namespace glaxnimate::model {

class OptionListPropertyBase : public BaseProperty
{
    Q_GADGET

public:
    enum OptionListFlags
    {
        NoFlags     = 0,
        LaxValues   = 1,
        FontCombo   = 2,
    };

    using BaseProperty::BaseProperty;

    virtual ~OptionListPropertyBase() = default;
    virtual QVariantList value_options() const = 0;

    int option_list_flags() const
    {
        return option_flags;
    }

protected:
    int option_flags;
};

template<class Type, class Container>
class OptionListProperty : public detail::PropertyTemplate<OptionListPropertyBase, Type>
{
public:
    OptionListProperty(
        Object* obj,
        const QString& name,
        Type default_value,
        PropertyCallback<Container> option_list,
        PropertyCallback<void, Type, Type> emitter = {},
        PropertyCallback<bool, Type> validator = {},
        PropertyTraits::Flags flags = PropertyTraits::Visual,
        int option_flags = 0
    )
        : detail::PropertyTemplate<OptionListPropertyBase, Type>(
            obj, name, std::move(default_value), std::move(emitter),
            std::move(validator), PropertyTraits::Flags(flags|PropertyTraits::OptionList)
        ),
        option_list(std::move(option_list))
    {
        this->option_flags = option_flags;
    }

    Container options() const
    {
        return option_list(this->object());
    }

    QVariantList value_options() const override
    {
        QVariantList list;
        for ( const auto& value : option_list(this->object()) )
            list.push_back(QVariant::fromValue(value));
        return list;
    }

private:
    PropertyCallback<Container> option_list;
};

} // namespace glaxnimate::model
