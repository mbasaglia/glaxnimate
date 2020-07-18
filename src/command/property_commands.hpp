#include <QUndoCommand>

#include "model/property.hpp"

namespace command {

class SetPropertyValue : public QUndoCommand
{
public:
    SetPropertyValue(model::BaseProperty* prop, const QVariant& value)
        : SetPropertyValue(prop, prop->value(), value)
    {}

    SetPropertyValue(model::BaseProperty* prop, const QVariant& before, const QVariant& after)
        : QUndoCommand(QObject::tr("Update %1").arg(prop->name())), prop(prop), before(before), after(after)
    {}

    void undo() override
    {
        prop->set_value(before);
    }

    void redo() override
    {
        prop->set_value(after);
    }



private:
    model::BaseProperty* prop;
    QVariant before;
    QVariant after;
};

} // namespace command
