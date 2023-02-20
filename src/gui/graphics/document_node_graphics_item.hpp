/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QGraphicsObject>

namespace glaxnimate::model {

class VisualNode;
class BaseProperty;

} // namespace glaxnimate::model

namespace glaxnimate::gui::graphics {

class DocumentNodeGraphicsItem : public QGraphicsObject
{
    Q_OBJECT

public:
    enum SelectionMode
    {
        None,
        Group,
        Shape,
    };

    explicit DocumentNodeGraphicsItem(model::VisualNode* node, QGraphicsItem* parent = nullptr);
    ~DocumentNodeGraphicsItem();

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;

    model::VisualNode* node() const
    {
        return node_;
    }

    SelectionMode selection_mode() const { return selection_mode_; }
    void set_selection_mode(SelectionMode selection_mode) { selection_mode_ = selection_mode; }

public slots:
    void set_visible(bool v)
    {
        visible = v;
        setVisible(visible && visible_permitted);
    }

    void set_visible_permitted(bool v)
    {
        visible_permitted = v;
        setVisible(visible && visible_permitted);
    }

    void shape_changed();

    void set_transform_matrix(const QTransform& t)
    {
        setTransform(t);
    }

    void set_opacity(qreal op)
    {
        setOpacity(op);
    }

protected:
    model::VisualNode* node_;
    bool visible_permitted = true;
    bool visible = true;
    SelectionMode selection_mode_ = SelectionMode::Shape;
    mutable QRectF rect_cache;
    mutable bool cache_dirty = true;
};

} // namespace glaxnimate::gui::graphics
