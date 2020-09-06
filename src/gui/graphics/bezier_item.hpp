#pragma once

#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include "handle.hpp"
#include "model/shapes/path.hpp"
#include "document_node_graphics_item.hpp"

namespace graphics {

class BezierPointItem : public QGraphicsObject
{
    Q_OBJECT
public:
    BezierPointItem(int index, const math::BezierPoint& point, QGraphicsItem* parent)
        : QGraphicsObject(parent), index(index)
    {
        connect(&tan_in, &MoveHandle::dragged, this, &BezierPointItem::tan_in_dragged);
        connect(&tan_out, &MoveHandle::dragged, this, &BezierPointItem::tan_out_dragged);
        connect(&pos, &MoveHandle::dragged, this, &BezierPointItem::on_dragged);
        pos.setParentItem(this);
        tan_in.setParentItem(&pos);
        tan_out.setParentItem(&pos);
        set_point(point);
    }

    QRectF boundingRect() const override
    {
        return QRectF(tan_in.pos() + pos.pos(), tan_out.pos() + pos.pos()).normalized();
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *) override
    {
        QPolygonF path;
        if ( tan_in.isVisible() )
            path.append(tan_in.pos());

        path.append(QPointF());

        if ( tan_out.isVisible() )
            path.append(tan_out.pos());

        if ( path.size() > 1 )
        {
            QPen p(option->palette.highlight(), 1);
            p.setCosmetic(true);
            painter->setPen(p);
            path.translate(pos.pos());
            painter->drawPolyline(path);
        }
    }

    void set_point(const math::BezierPoint& p)
    {
        pos.setPos(p.pos);
        tan_in.setPos(p.relative_tan_in());
        tan_out.setPos(p.relative_tan_out());
        set_point_type(p.type);
    }

    void set_index(int index)
    {
        this->index = index;
    }

    void set_point_type(math::BezierPointType type)
    {
        switch ( this->type = type )
        {
            case math::BezierPointType::Corner:
                pos.change_shape(MoveHandle::Diamond, 8);
                break;

            case math::BezierPointType::Smooth:
            {
                math::PolarVector<QPointF> p_in(tan_in.pos());
                math::PolarVector<QPointF> p_out(tan_out.pos());
                if ( p_in.length != p_out.length )
                {
                    p_in.length = p_out.length = (p_in.length + p_out.length) / 2;
                    tan_in.setPos(p_in.to_cartesian());
                    tan_out.setPos(p_out.to_cartesian());
                }
                pos.change_shape(MoveHandle::Square, 6);
            }
            break;

            case math::BezierPointType::Symmetrical:
            {
                math::PolarVector<QPointF> p_in(tan_in.pos());
                math::PolarVector<QPointF> p_out(tan_out.pos());

                p_in.length = p_out.length = (p_in.length + p_out.length) / 2;
                p_in.angle = p_out.angle = (p_in.angle + p_out.angle) / 2;

                tan_in.setPos(p_in.to_cartesian());
                tan_out.setPos(p_out.to_cartesian());

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
    void dragged(int index, const math::BezierPoint& point);

private slots:
    void tan_in_dragged(const QPointF& p)
    {
        tan_out.setPos(math::BezierPoint::drag_tangent(p, tan_out.pos(), pos.pos(), type));
        on_dragged();
    }

    void tan_out_dragged(const QPointF& p)
    {
        tan_in.setPos(math::BezierPoint::drag_tangent(p, tan_in.pos(), pos.pos(), type));
        on_dragged();
    }

    void on_dragged()
    {
        emit dragged(index, math::BezierPoint::from_relative(pos.pos(), tan_in.pos(), tan_out.pos(), type));
    }

private:
    MoveHandle pos{nullptr, MoveHandle::Any, MoveHandle::Diamond, 8};
    MoveHandle tan_in{nullptr, MoveHandle::Any, MoveHandle::Circle, 6};
    MoveHandle tan_out{nullptr, MoveHandle::Any, MoveHandle::Circle, 6};

    math::BezierPointType type;
    int index;
};


class BezierItem : public QGraphicsObject
{
    Q_OBJECT

public:
    const math::Bezier& bezier()
    {
        return bezier_;
    }

    QRectF boundingRect() const override
    {
        return {};
    }

    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget*) override
    {}

    int size() const
    {
        return items.size();
    }

    void set_type(int index, math::BezierPointType type)
    {
        items[index]->set_point_type(type);
    }

public slots:
    void set_bezier(const math::Bezier& bez)
    {
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


        items.front()->show_tan_in(!bezier_.closed());
        items.back()->show_tan_out(!bezier_.closed());

//         prepareGeometryChange();
    }

    void reverse()
    {
        std::reverse(items.begin(), items.end());
        for ( int i = 0; i < int(items.size()); i++ )
        {
            items[i]->set_index(i);
        }
    }

    void remove_point(int index)
    {
        bezier_.points().erase(bezier_.begin() + index);
        items.erase(items.begin() + index);
        if ( !bezier_.closed() )
            items.back()->show_tan_out(false);
    }

    void pop_back()
    {
        if ( !bezier_.empty() )
            remove_point(bezier_.size() - 1);
    }

signals:
    void bezier_changed(math::Bezier& bez);

private slots:
    void on_dragged(int index, const math::BezierPoint& point)
    {
        bezier_.set_point(index, point);
        emit bezier_changed(bezier_);
//         prepareGeometryChange();
    }

private:
    void do_add_point(int index)
    {
        items.push_back(std::make_unique<BezierPointItem>(index, bezier_[index], this));
        connect(items.back().get(), &BezierPointItem::dragged, this, &BezierItem::on_dragged);
    }

    math::Bezier bezier_;
    std::vector<std::unique_ptr<BezierPointItem>> items;
};


} // namespace graphics
