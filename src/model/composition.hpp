#pragma once

#include "layers.hpp"

namespace model {

class Composition : public ObjectBase<Composition, Object>
{
public:
    ObjectListProperty<Layer> layers{this, "layers", "layers"};

    ChildLayerView top_level() const
    {
        return ChildLayerView(this, nullptr);
    }
};

} // namespace model

