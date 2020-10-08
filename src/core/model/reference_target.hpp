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
    /**
     * @brief Name of the node, used to display it in the UI
     */
    GLAXNIMATE_PROPERTY(QString, name, "", &ReferenceTarget::name_changed)

public:
    explicit ReferenceTarget(model::Document* document)
     : Object ( document )
     {
        uuid.set_value(QUuid::createUuid());
     }

    virtual QIcon reftarget_icon() const = 0;

    /**
     * \brief Recursively updates uuid
     */
    void refresh_uuid();

    QString object_name() const override;

signals:
    void name_changed(const QString&);
};

} // namespace model
