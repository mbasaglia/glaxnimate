#pragma once

#include "layers.hpp"

namespace model {

class Composition : public Object
{
public:
    ObjectListProperty<Layer> layers{this, "layers", "layers"};
};

} // namespace model

