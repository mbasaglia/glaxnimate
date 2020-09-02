#pragma once

#include "layer.hpp"

namespace model {


class EmptyLayer : public detail::BaseLayerProps<EmptyLayer>
{
    GLAXNIMATE_OBJECT

public:
    using Ctor::Ctor;

    QIcon docnode_icon() const override
    {
        return QIcon::fromTheme("transform-move");
    }

    QString type_name_human() const override { return tr("Empty Layer"); }
};


} // namespace model
