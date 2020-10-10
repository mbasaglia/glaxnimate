#pragma once

#include <QVector>

#include "command/base.hpp"
#include "model/property/property.hpp"

namespace command {

class SetPropertyValue : public MergeableCommand<Id::SetPropertyValue, SetPropertyValue>
{
public:
    SetPropertyValue(model::BaseProperty* prop, const QVariant& value, bool commit = true)
        : SetPropertyValue(prop, prop->value(), value, commit)
    {}

    SetPropertyValue(model::BaseProperty* prop, const QVariant& before, const QVariant& after, bool commit = true, const QString& name = {})
        : Parent(name.isEmpty() ? QObject::tr("Update %1").arg(prop->name()) : name, commit),
            prop(prop),
            before(before),
            after(after)
    {}

    void undo() override
    {
        prop->set_value(before);
    }

    void redo() override
    {
        prop->set_value(after);
    }


    bool merge_with(const SetPropertyValue& other)
    {
        if ( other.prop != prop )
            return false;
        after = other.after;
        return true;
    }

private:
    model::BaseProperty* prop;
    QVariant before;
    QVariant after;
};


class SetMultipleProperties : public MergeableCommand<Id::SetMultipleProperties, SetMultipleProperties>
{
public:
    template<class... Args>
    SetMultipleProperties(
        const QString& name,
        bool commit,
        const QVector<model::BaseProperty*>& props,
        Args... vals
    ) : SetMultipleProperties(name, props, {}, {QVariant::fromValue(vals)...}, commit)
    {}

    /**
     * \pre props.size() == after.size() && (props.size() == before.size() || before.empty())
     *
     * If before.empty() it will be populated by the properties
     */
    SetMultipleProperties(
        const QString& name,
        const QVector<model::BaseProperty*>& props,
        const QVariantList& before,
        const QVariantList& after,
        bool commit
    )
        : Parent(name, commit), props(props), before(before), after(after)
    {
        if ( before.empty() )
            for ( auto prop : props )
                this->before.push_back(prop->value());
    }

    void undo() override
    {
        for ( int i = 0; i < props.size(); i++ )
            props[i]->set_value(before[i]);
    }

    void redo() override
    {
        for ( int i = 0; i < props.size(); i++ )
            props[i]->set_value(after[i]);
    }


    bool merge_with(const SetMultipleProperties& other)
    {
        if ( other.props.size() != props.size() )
            return false;

        for ( int i = 0; i < props.size(); i++ )
            if ( props[i] != other.props[i] )
                return false;

        after = other.after;
        return true;
    }

private:
    QVector<model::BaseProperty*> props;
    QVariantList before;
    QVariantList after;
};

} // namespace command
