#pragma once

#include <QIcon>
#include <QUuid>

#include "model/object.hpp"
#include "model/property/property.hpp"

namespace model {


class ReferenceTarget : public Object
{
    Q_OBJECT

    /**
     * @brief Unique identifier for the node
     */
    GLAXNIMATE_PROPERTY_RO(QUuid, uuid, {})

public:
    explicit ReferenceTarget(model::Document* document)
     : Object ( document )
     {
        uuid.set_value(QUuid::createUuid());
     }

    virtual QIcon reftarget_icon() const = 0;
};

} // namespace model
