#pragma once

#include <QGraphicsObject>

namespace glaxnimate::gui::graphics {

namespace Types{enum Types
{
    BaseValue = QGraphicsItem::UserType,
    BezierItem,
};}

template<int TypeV, class Base=QGraphicsObject>
class TypedItem : public Base
{
protected:
    using Ctor = TypedItem;
public:
    using Base::Base;
    enum { Type = TypeV };
    int type() const override { return TypeV; }
};

} // namespace glaxnimate::gui::graphics
