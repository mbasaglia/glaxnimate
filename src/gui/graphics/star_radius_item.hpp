/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QPen>

#include "handle.hpp"
#include "model/document.hpp"
#include "command/animation_commands.hpp"
#include "model/shapes/polystar.hpp"
#include "math/math.hpp"

namespace glaxnimate::gui::graphics {

class StarRadiusItem : public QGraphicsObject
{
public:
    StarRadiusItem(model::PolyStar* shape)
        : shape(shape),
        handle_outer(this, MoveHandle::Any, MoveHandle::Square),
        handle_inner(this, MoveHandle::Any, MoveHandle::Diamond)
    {
        adjust_pos();
        handle_inner.setVisible(shape->type.get() == model::PolyStar::Star);
        connect(&handle_outer, &MoveHandle::dragged, this, &StarRadiusItem::on_drag_outer);
        connect(&handle_outer, &MoveHandle::drag_finished, this, &StarRadiusItem::on_commit_outer);
        connect(&handle_inner, &MoveHandle::dragged, this, &StarRadiusItem::on_drag_inner);
        connect(&handle_inner, &MoveHandle::drag_finished, this, &StarRadiusItem::on_commit_inner);
        connect(shape, &model::Object::property_changed, this, &StarRadiusItem::on_prop_changed);

        handle_inner.set_associated_property(&shape->inner_radius);
        handle_outer.set_associated_property(&shape->outer_radius);
    }

    QRectF boundingRect() const override
    {
        return shape->local_bounding_rect(shape->time());
    }


    void paint(QPainter* painter, const QStyleOptionGraphicsItem * opt, QWidget *) override
    {
        painter->save();

        QPen pen(opt->palette.color(QPalette::Highlight), 1);
        pen.setCosmetic(true);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);

        QPainterPath p;
        shape->to_bezier(shape->time()).add_to_painter_path(p);
        painter->drawPath(p);

        painter->restore();
    }


private slots:
    void on_drag_outer(const QPointF& p, Qt::KeyboardModifiers mod)
    {
        math::PolarVector<QPointF> pv = p - shape->position.get();

        qreal angle;
        if ( mod & Qt::ControlModifier )
            angle = shape->angle.get();
        else
            angle = math::rad2deg(pv.angle + math::pi / 2);

        push_outer(pv.length, angle, false);
    }

    void on_commit_outer()
    {
        push_outer(shape->outer_radius.value(), shape->angle.value(), true);
    }

    void on_drag_inner(const QPointF& p)
    {
        push_inner(math::length(p - shape->position.get()), false);
    }

    void on_commit_inner()
    {
        push_inner(shape->inner_radius.value(), true);
    }

    void on_prop_changed(const model::BaseProperty* prop)
    {
        if ( prop->traits().flags & model::PropertyTraits::Visual )
        {
            if ( prop == &shape->outer_radius || prop == &shape->inner_radius || prop == &shape->position )
                prepareGeometryChange();
            else if ( prop == &shape->type )
                handle_inner.setVisible(shape->type.get() == model::PolyStar::Star);

            adjust_pos();
        }
    }

private:
    void push_inner(const QVariant& val, bool commit)
    {
        shape->push_command(new command::SetMultipleAnimated(
            shape->inner_radius.name(), {&shape->inner_radius}, {shape->inner_radius.value()}, {val}, commit
        ));
    }

    void push_outer(const QVariant& radius, const QVariant& angle, bool commit)
    {
        shape->push_command(new command::SetMultipleAnimated(
            shape->outer_radius.name(),
            {&shape->outer_radius, &shape->angle},
            {shape->outer_radius.value(), shape->angle.value()},
            {radius, angle},
            commit
        ));
    }

    void adjust_pos()
    {
        qreal angle = math::deg2rad(shape->angle.get()) - math::pi / 2;

        handle_outer.setPos(
            shape->position.get() +
            math::PolarVector<QPointF>(shape->outer_radius.get(), angle).to_cartesian()
        );

        if ( handle_inner.isVisible() )
        {
            handle_inner.setPos(
                shape->position.get() +
                math::PolarVector<QPointF>(
                    shape->inner_radius.get(),
                    angle + math::pi / shape->points.get()
                ).to_cartesian()
            );
        }
    }

    model::PolyStar* shape;
    MoveHandle handle_outer;
    MoveHandle handle_inner;
};

} // namespace glaxnimate::gui::graphics

