/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QStyleOptionGraphicsItem>
#include <QPainter>

#include "handle.hpp"
#include "model/document.hpp"
#include "command/animation_commands.hpp"
#include "math/geom.hpp"

namespace glaxnimate::gui::graphics {

class SizePosItem : public QGraphicsObject
{
public:
    SizePosItem(
        model::AnimatedProperty<QSizeF>* size,
        model::AnimatedProperty<QPointF>* pos
    ) : size(size),
        pos(pos),
        handle_tl(this, MoveHandle::Any, MoveHandle::Square),
        handle_br(this, MoveHandle::Any, MoveHandle::Square)
    {
        connect(&handle_tl, &MoveHandle::drag_starting, this, &SizePosItem::on_drag_start);
        connect(&handle_br, &MoveHandle::drag_starting, this, &SizePosItem::on_drag_start);
        connect(&handle_tl, &MoveHandle::dragged, this, &SizePosItem::on_drag_tl);
        connect(&handle_br, &MoveHandle::dragged, this, &SizePosItem::on_drag_br);
        connect(&handle_tl, &MoveHandle::drag_finished, this, &SizePosItem::on_commit);
        connect(&handle_br, &MoveHandle::drag_finished, this, &SizePosItem::on_commit);
        connect(size->object(), &model::Object::property_changed, this, &SizePosItem::on_prop_changed);
        reset_rect();

        handle_tl.set_associated_properties({size, pos});
        handle_br.set_associated_properties({size, pos});
    }

    QRectF boundingRect() const override
    {
        return rect;
    }


    void paint(QPainter* painter, const QStyleOptionGraphicsItem * opt, QWidget *) override
    {
        painter->save();
        QPen pen(opt->palette.color(QPalette::Highlight), 1);
        pen.setCosmetic(true);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(rect);
        painter->restore();
    }

private slots:
    void on_drag_tl(QPointF p, Qt::KeyboardModifiers modifiers)
    {
        if ( modifiers & Qt::ControlModifier )
        {
            find_candidate(p, start_tl);
        }

        if ( p.x() > rect.right() )
            p.setX(rect.right());
        if ( p.y() > rect.bottom() )
            p.setY(rect.bottom());

        if ( modifiers & Qt::ShiftModifier )
        {
            rect = QRectF(
                p,
                2*rect.center() - p
            );
        }
        else
        {
            rect.setTopLeft(p);
        }

        push(false);
    }

    void on_drag_br(QPointF p, Qt::KeyboardModifiers modifiers)
    {
        if ( modifiers & Qt::ControlModifier )
        {
            find_candidate(p, start_br);
        }

        if ( p.x() < rect.left() )
            p.setX(rect.left());
        if ( p.y() < rect.top() )
            p.setY(rect.top());

        if ( modifiers & Qt::ShiftModifier )
        {
            rect = QRectF(
                2*rect.center() - p,
                p
            );
        }
        else
        {
            rect.setBottomRight(p);
        }

        push(false);
    }

    void on_drag_start()
    {
        start_tl = handle_tl.pos();
        start_br = handle_br.pos();
    }

    void on_commit()
    {
        push(true);
    }

    void on_prop_changed(const model::BaseProperty* prop)
    {
        if ( prop == size || prop == pos)
            reset_rect();
    }

private:
    void push(bool commit)
    {
        std::vector<model::AnimatableBase*> props;
        QVariantList before;
        QVariantList after;

        props.push_back(size);
        before.push_back(size->value());
        after.push_back(rect.size());

        auto pos_before = pos->get();
        auto& stack = size->object()->document()->undo_stack();

        // Check we should only update the size
        bool skip_pos = qFuzzyCompare(pos_before.x(), rect.center().x()) && qFuzzyCompare(pos_before.y(), rect.center().y());
        if ( skip_pos )
        {
            int undo_index = stack.index() - 1;
            if ( undo_index > 0 )
            {
                auto last_command = stack.command(undo_index);
                if ( last_command->id() == int(command::Id::SetMultipleAnimated) )
                {
                    auto cmd = static_cast<const command::SetMultipleAnimated*>(last_command);
                    if ( cmd->properties().size() == 2 && cmd->properties()[0] == size && cmd->properties()[1] == pos )
                        skip_pos = false;
                }
            }
        }

        if ( !skip_pos )
        {
            props.push_back(pos);
            before.push_back(pos->value());
            after.push_back(rect.center());
        }

        stack.push(new command::SetMultipleAnimated(size->name(), props, before, after, commit));
        prepareGeometryChange();
    }

    void reset_rect()
    {
        QSizeF sz = size->get();
        rect = QRectF(pos->get() - QPointF(sz.width(), sz.height())/2, sz);
        reset_handles();
        prepareGeometryChange();
    }

    void reset_handles()
    {
        handle_tl.setPos(rect.topLeft());
        handle_br.setPos(rect.bottomRight());
    }

    void find_candidate(QPointF& p, const QPointF start)
    {
        QPointF candidates[] = {
            QPointF{p.x(), start.y()},
            QPointF{start.x(), p.y()},
            math::line_closest_point(start_tl, start_br, p)
        };
        qreal min_dist = std::numeric_limits<qreal>::max();
        QPointF selected;
        for ( const QPointF& cand : candidates )
        {
            qreal dist = math::length_squared(p - cand);
            if ( dist < min_dist )
            {
                selected = cand;
                min_dist = dist;
            }
        }

        p = selected;
    }

    model::AnimatedProperty<QSizeF>* size;
    model::AnimatedProperty<QPointF>* pos;
    QRectF rect;
    QPointF start_tl;
    QPointF start_br;
    MoveHandle handle_tl;
    MoveHandle handle_br;
};

} // namespace glaxnimate::gui::graphics
