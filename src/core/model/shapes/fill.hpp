#pragma once

#include <QBrush>
#include <QPainter>

#include "styler.hpp"
#include "model/animation/animatable.hpp"

namespace model {

class Fill : public Styler
{
    GLAXNIMATE_OBJECT(Fill)

public:
    enum Rule
    {
        NonZero = Qt::WindingFill,
        EvenOdd = Qt::OddEvenFill,
    };

private:
    Q_ENUM(Rule);

    GLAXNIMATE_PROPERTY(Rule, fill_rule, NonZero, nullptr, nullptr, PropertyTraits::Visual)

public:
    using Styler::Styler;

    QRectF local_bounding_rect(FrameTime t) const override
    {
        return collect_shapes(t).bounding_box();
    }
    QIcon docnode_icon() const override
    {
        return QIcon::fromTheme("format-fill-color");
    }

    QString type_name_human() const override
    {
        return tr("Fill");
    }

protected:
    void on_paint(QPainter* p, FrameTime t, PaintMode) const override;
};


} // namespace model
