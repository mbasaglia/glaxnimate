#pragma once

#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include "handle.hpp"
#include "model/shapes/path.hpp"
#include "document_node_graphics_item.hpp"
#include "math/geom.hpp"

namespace graphics {

class BezierPointItem : public QGraphicsObject
{
    Q_OBJECT
public:
    BezierPointItem(int index, const math::BezierPoint& point, QGraphicsItem* parent)
        : QGraphicsObject(parent), index(index), point(point)
    {
        connect(&tan_in, &MoveHandle::dragged, this, &BezierPointItem::tan_in_dragged);
        connect(&tan_out, &MoveHandle::dragged, this, &BezierPointItem::tan_out_dragged);
        connect(&pos, &MoveHandle::dragged, this, &BezierPointItem::pos_dragged);
        connect(&pos, &MoveHandle::clicked, this, &BezierPointItem::pos_clicked);
        pos.setParentItem(this);
        tan_in.setParentItem(this);
        tan_out.setParentItem(this);
        set_point(point);
    }

    QRectF boundingRect() const override
    {
        return QRectF(tan_in.pos(), tan_out.pos()).normalized();
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *) override
    {
        QPolygonF path;
        if ( tan_in.isVisible() )
            path.append(tan_in.pos());

        path.append(pos.pos());

        if ( tan_out.isVisible() )
            path.append(tan_out.pos());

        if ( path.size() > 1 )
        {
            QPen p(option->palette.highlight(), 1);
            p.setCosmetic(true);
            painter->setPen(p);
            painter->drawPolyline(path);
        }
    }

    void set_point(const math::BezierPoint& p)
    {
        pos.setPos(p.pos);
        tan_in.setPos(p.tan_in);
        tan_out.setPos(p.tan_out);
        set_point_type(p.type);
    }

    void set_index(int index)
    {
        this->index = index;
    }

    void set_point_type(math::BezierPointType type)
    {
        switch ( point.type = type )
        {
            case math::BezierPointType::Corner:
                pos.change_shape(MoveHandle::Diamond, 8);
                break;

            case math::BezierPointType::Smooth:
            {
                QPointF p = pos.pos();
                math::PolarVector<QPointF> p_in(tan_in.pos() - p);
                math::PolarVector<QPointF> p_out(tan_out.pos() - p);
                if ( p_in.length != p_out.length )
                {
                    p_in.length = p_out.length = (p_in.length + p_out.length) / 2;
                    tan_in.setPos(p_in.to_cartesian() + p);
                    tan_out.setPos(p_out.to_cartesian() + p);
                }
                pos.change_shape(MoveHandle::Square, 6);
            }
            break;

            case math::BezierPointType::Symmetrical:
            {
                QPointF p = pos.pos();
                math::PolarVector<QPointF> p_in(tan_in.pos() - p);
                math::PolarVector<QPointF> p_out(tan_out.pos() - p);

                p_in.length = p_out.length = (p_in.length + p_out.length) / 2;
                qreal in_angle = (p_in.angle + p_out.angle + M_PI) / 2;
                p_in.angle = in_angle;
                p_out.angle = in_angle + M_PI;

                tan_in.setPos(p_in.to_cartesian() + p);
                tan_out.setPos(p_out.to_cartesian() + p);

                pos.change_shape(MoveHandle::Circle, 7);
            }
            break;
        }
    }

    void show_tan_in(bool show)
    {
        tan_in.setVisible(show);
        update();
    }

    void show_tan_out(bool show)
    {
        tan_out.setVisible(show);
        update();
    }

signals:
    void modified(int index, const math::BezierPoint& point);

private slots:
    void tan_in_dragged(const QPointF& p, Qt::KeyboardModifiers mods)
    {
        if ( point.type == math::Corner && (mods & Qt::ShiftModifier) )
        {
            drag_preserve_angle(point.tan_in, point.tan_out, p);
        }
        else if ( mods & Qt::ControlModifier )
        {
            QPointF constrained = math::line_closest_point(point.pos, point.tan_in, p);
            point.drag_tan_in(constrained);
            tan_in.setPos(constrained);
        }
        else
        {
            point.drag_tan_in(p);
        }

        tan_out.setPos(point.tan_out);
        on_modified();
    }

    void tan_out_dragged(const QPointF& p, Qt::KeyboardModifiers mods)
    {
        if ( point.type == math::Corner && (mods & Qt::ShiftModifier) )
        {
            drag_preserve_angle(point.tan_out, point.tan_in, p);
        }
        else if ( mods & Qt::ControlModifier )
        {
            QPointF constrained = math::line_closest_point(point.pos, point.tan_out, p);
            point.drag_tan_out(constrained);
            tan_out.setPos(constrained);
        }
        else
        {
            point.drag_tan_out(p);
        }

        tan_in.setPos(point.tan_in);
        on_modified();
    }

    void pos_dragged(const QPointF& p)
    {
        auto delta = p - point.pos;
        tan_in.setPos(point.tan_in += delta);
        tan_out.setPos(point.tan_out += delta);
        point.pos = p;
        on_modified();
    }

    void on_modified()
    {
        emit modified(index, point);
    }

    void pos_clicked(Qt::KeyboardModifiers mod)
    {
        if ( mod & Qt::ControlModifier )
        {
            if ( point.type == math::BezierPointType::Corner )
                set_point_type(math::BezierPointType::Smooth);
            else
                set_point_type(math::BezierPointType::Corner);

            on_modified();
        }
    }

private:
    void drag_preserve_angle(QPointF& dragged, QPointF& other, const QPointF& dragged_new)
    {
        QPointF rel_other = other - point.pos;
        QPointF rel_dragged_old = dragged - point.pos;
        QPointF rel_dragged_new = dragged_new - point.pos;
        qreal angle_between = math::angle(rel_dragged_old) - math::angle(rel_other);
        qreal angle = math::angle(rel_dragged_new) - angle_between;
        qreal length = math::length(rel_other);

        dragged = dragged_new;
        other = point.pos + math::PolarVector<QPointF>(length, angle).to_cartesian();
    }

    MoveHandle pos{nullptr, MoveHandle::Any, MoveHandle::Diamond, 8};
    MoveHandle tan_in{nullptr, MoveHandle::Any, MoveHandle::Circle, 6};
    MoveHandle tan_out{nullptr, MoveHandle::Any, MoveHandle::Circle, 6};

    int index;
    math::BezierPoint point;
};


class BezierItem : public QGraphicsObject
{
    Q_OBJECT

public:
    BezierItem(model::Path* node, QGraphicsItem* parent=nullptr)
    : QGraphicsObject(parent), node(node)
    {
        set_bezier(node->shape.get());
        connect(node, &model::Path::shape_changed, this, &BezierItem::set_bezier);
    }

    QRectF boundingRect() const override
    {
        return bezier_.bounding_box();
    }

    void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget*) override
    {
        QPainterPath path;
        bezier_.add_to_painter_path(path);
        QPen pen(option->palette.highlight(), 1);
        pen.setCosmetic(true);
        painter->setPen(pen);
        painter->drawPath(path);
    }

    void set_type(int index, math::BezierPointType type)
    {
        items[index]->set_point_type(type);
    }

public slots:
    void set_bezier(const math::Bezier& bez)
    {
        if ( updating )
            return;

        int old_size = bezier_.size();
        bezier_ = bez;

        if ( old_size > bezier_.size() )
            items.erase(items.begin() + bezier_.size(), items.end());

        for ( int i = 0; i < int(items.size()); i++ )
            items[i]->set_point(bezier_[i]);

        if ( old_size < bezier_.size() )
        {
            if ( !items.empty() )
                items.back()->show_tan_out(true);
            for ( int i = old_size; i < bezier_.size(); i++ )
                do_add_point(i);
        }

        if ( !bezier_.empty() )
        {
            items.front()->show_tan_in(bezier_.closed());
            items.back()->show_tan_out(bezier_.closed());
        }

        prepareGeometryChange();
    }

//     void reverse()
//     {
//         std::reverse(items.begin(), items.end());
//         for ( int i = 0; i < int(items.size()); i++ )
//         {
//             items[i]->set_index(i);
//         }
//     }

    void remove_point(int index)
    {
        bezier_.points().erase(bezier_.begin() + index);
        items.erase(items.begin() + index);
        if ( !bezier_.closed() )
            items.back()->show_tan_out(false);
    }

private slots:
    void on_dragged(int index, const math::BezierPoint& point)
    {
        bezier_.set_point(index, point);
        updating = true;
        node->shape.set(bezier_);
        updating = false;
        prepareGeometryChange();
    }

private:
    void do_add_point(int index)
    {
        items.push_back(std::make_unique<BezierPointItem>(index, bezier_[index], this));
        connect(items.back().get(), &BezierPointItem::modified, this, &BezierItem::on_dragged);
        if ( !bezier_.closed() && index == int(items.size()) - 1 && items.size() > 2)
            items[index-1]->show_tan_out(true);

    }

    math::Bezier bezier_;
    std::vector<std::unique_ptr<BezierPointItem>> items;
    model::Path* node;
    bool updating = false;
};


} // namespace graphics
