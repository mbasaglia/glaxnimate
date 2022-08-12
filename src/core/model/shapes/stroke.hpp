#pragma once

#include <set>

#include <QBrush>
#include <QPainter>

#include "styler.hpp"
#include "model/animation/animatable.hpp"

namespace glaxnimate::model {

class Stroke : public StaticOverrides<Stroke, Styler>
{
    GLAXNIMATE_OBJECT(Stroke)

public:
    enum Cap
    {
        ButtCap = Qt::FlatCap,
        RoundCap = Qt::RoundCap,
        SquareCap = Qt::SquareCap,
    };

    enum Join
    {
        MiterJoin = Qt::MiterJoin,
        RoundJoin = Qt::RoundJoin,
        BevelJoin = Qt::BevelJoin,
    };

private:
    Q_ENUM(Cap);
    Q_ENUM(Join);

    GLAXNIMATE_ANIMATABLE(float, width, 1, {}, 0)
    GLAXNIMATE_PROPERTY(Cap, cap, RoundCap, nullptr, nullptr, PropertyTraits::Visual)
    GLAXNIMATE_PROPERTY(Join, join, RoundJoin, nullptr, nullptr, PropertyTraits::Visual)
    GLAXNIMATE_PROPERTY(float, miter_limit, 0, nullptr, nullptr, PropertyTraits::Visual)

public:
    using Ctor::Ctor;

    QRectF local_bounding_rect(FrameTime t) const override
    {
        if ( !visible.get() )
            return {};
        qreal half_width = width.get_at(t) / 2;
        return collect_shapes(t, {}).bounding_box().adjusted(
            -half_width, -half_width, half_width, half_width
        );
    }


    static QIcon static_tree_icon()
    {
        return QIcon::fromTheme("format-stroke-color");
    }

    static QString static_type_name_human()
    {
        return tr("Stroke");
    }

    void set_pen_style(const QPen& p);
    void set_pen_style_undoable(const QPen& p);

    QPainterPath to_painter_path(FrameTime t) const override;

protected:
    void on_paint(QPainter* p, FrameTime t, PaintMode, model::Modifier* modifier) const override;
};

} // namespace glaxnimate::model

