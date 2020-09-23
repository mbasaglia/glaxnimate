#pragma once

#include <QIcon>

#include "model/object.hpp"

namespace model {


class ReferenceTarget : public Object
{
public:
    using Object::Object;

    virtual QIcon reftarget_icon() const = 0;
};

} // namespace model
