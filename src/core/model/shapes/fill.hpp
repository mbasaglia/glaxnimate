#pragma once
#ifdef _HAX_FUCKING_QMAKE_I_HATE_YOU_
    Q_OBJECT
#endif

#include <QBrush>
#include <QPainter>

#include "styler.hpp"
#include "model/animation/animatable.hpp"

namespace model {

class Fill : public StaticOverrides<Fill, Styler>
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
    using Ctor::Ctor;

    QRectF local_bounding_rect(FrameTime t) const override
    {
        return collect_shapes(t, {}).bounding_box();
    }

    static QIcon static_tree_icon()
    {
        return QIcon::fromTheme("format-fill-color");
    }

    static QString static_type_name_human()
    {
        return tr("Fill");
    }

    QPainterPath to_painter_path(FrameTime t) const override;

protected:
    void on_paint(QPainter* p, FrameTime t, PaintMode, model::Modifier* modifier) const override;
};


} // namespace model
