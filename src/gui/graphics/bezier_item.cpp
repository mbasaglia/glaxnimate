#include "bezier_item.hpp"

#include "math/geom.hpp"

#include "command/animation_commands.hpp"

graphics::BezierPointItem::BezierPointItem(int index, const math::BezierPoint& point, QGraphicsItem* parent)
: QGraphicsObject(parent), index_(index), point_(point)
{
    connect(&tan_in, &MoveHandle::dragged, this, &BezierPointItem::tan_in_dragged);
    connect(&tan_in, &MoveHandle::drag_finished, this, &BezierPointItem::on_commit);
    connect(&tan_out, &MoveHandle::dragged, this, &BezierPointItem::tan_out_dragged);
    connect(&tan_out, &MoveHandle::drag_finished, this, &BezierPointItem::on_commit);
    connect(&pos, &MoveHandle::dragged, this, &BezierPointItem::pos_dragged);
    connect(&pos, &MoveHandle::drag_finished, this, &BezierPointItem::on_commit);
    connect(&pos, &MoveHandle::clicked, this, &BezierPointItem::pos_clicked);

    pos.setParentItem(this);
    tan_in.setParentItem(this);
    tan_out.setParentItem(this);

    pos.set_role(MoveHandle::Vertex);
    tan_in.set_role(MoveHandle::Tangent);
    tan_out.set_role(MoveHandle::Tangent);

    set_point(point);
}

const math::BezierPoint & graphics::BezierPointItem::point() const
{
    return point_;
}

void graphics::BezierPointItem::modify(const math::BezierPoint& pt, const QString& undo_name)
{
    set_point(pt);
    on_modified(true, undo_name);
}


QRectF graphics::BezierPointItem::boundingRect() const
{
    return QRectF(tan_in.pos(), tan_out.pos()).normalized();
}

void graphics::BezierPointItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
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

void graphics::BezierPointItem::set_point(const math::BezierPoint& p)
{
    point_ = p;
    pos.setPos(p.pos);
    tan_in.setPos(p.tan_in);
    tan_out.setPos(p.tan_out);

    switch ( point_.type )
    {
        case math::BezierPointType::Corner:
            pos.change_shape(MoveHandle::Diamond, 8);
            break;
        case math::BezierPointType::Smooth:
            pos.change_shape(MoveHandle::Square, 6);
            break;
        case math::BezierPointType::Symmetrical:
            pos.change_shape(MoveHandle::Circle, 7);
            break;
    }
}

void graphics::BezierPointItem::set_index(int index)
{
    index_ = index;
}

void graphics::BezierPointItem::set_point_type(math::BezierPointType type)
{
    point_.type = type;
    point_.adjust_handles_from_type();
    set_point(point_);
    on_modified(true);
}

void graphics::BezierPointItem::show_tan_in(bool show)
{
    tan_in.setVisible(show);
    update();
}

void graphics::BezierPointItem::show_tan_out(bool show)
{
    tan_out.setVisible(show);
    update();
}
void graphics::BezierPointItem::tan_in_dragged(const QPointF& p, Qt::KeyboardModifiers mods)
{
    if ( point_.type == math::Corner && (mods & Qt::ShiftModifier) )
    {
        drag_preserve_angle(point_.tan_in, point_.tan_out, p);
    }
    else if ( mods & Qt::ControlModifier )
    {
        QPointF constrained = math::line_closest_point(point_.pos, point_.tan_in, p);
        point_.drag_tan_in(constrained);
        tan_in.setPos(constrained);
    }
    else
    {
        point_.drag_tan_in(p);
    }

    tan_out.setPos(point_.tan_out);
    on_modified(false);
}

void graphics::BezierPointItem::tan_out_dragged(const QPointF& p, Qt::KeyboardModifiers mods)
{
    if ( point_.type == math::Corner && (mods & Qt::ShiftModifier) )
    {
        drag_preserve_angle(point_.tan_out, point_.tan_in, p);
    }
    else if ( mods & Qt::ControlModifier )
    {
        QPointF constrained = math::line_closest_point(point_.pos, point_.tan_out, p);
        point_.drag_tan_out(constrained);
        tan_out.setPos(constrained);
    }
    else
    {
        point_.drag_tan_out(p);
    }

    tan_in.setPos(point_.tan_in);
    on_modified(false);
}

void graphics::BezierPointItem::pos_dragged(const QPointF& p)
{
    auto delta = p - point_.pos;
    tan_in.setPos(point_.tan_in += delta);
    tan_out.setPos(point_.tan_out += delta);
    point_.pos = p;
    on_modified(false);
}

void graphics::BezierPointItem::on_modified(bool commit, const QString& name)
{
    emit modified(index_, point_, commit, name);
}

void graphics::BezierPointItem::on_commit()
{
    on_modified(true);
}


void graphics::BezierPointItem::pos_clicked(Qt::KeyboardModifiers mod)
{
    if ( mod & Qt::ControlModifier )
    {
        if ( point_.type == math::BezierPointType::Corner )
            set_point_type(math::BezierPointType::Smooth);
        else
            set_point_type(math::BezierPointType::Corner);
    }
}

void graphics::BezierPointItem::drag_preserve_angle(QPointF& dragged, QPointF& other, const QPointF& dragged_new)
{
    QPointF rel_other = other - point_.pos;
    QPointF rel_dragged_old = dragged - point_.pos;
    QPointF rel_dragged_new = dragged_new - point_.pos;
    qreal angle_between = math::angle(rel_dragged_old) - math::angle(rel_other);
    qreal angle = math::angle(rel_dragged_new) - angle_between;
    qreal length = math::length(rel_other);

    dragged = dragged_new;
    other = point_.pos + math::PolarVector<QPointF>(length, angle).to_cartesian();
}


graphics::BezierItem * graphics::BezierPointItem::parent_editor() const
{
    return static_cast<graphics::BezierItem *>(parentItem());
}

int graphics::BezierPointItem::index() const
{
    return index_;
}



graphics::BezierItem::BezierItem(model::Path* node, QGraphicsItem* parent)
: QGraphicsObject(parent), node(node)
{
    set_bezier(node->shape.get());
    connect(node, &model::Path::shape_changed, this, &BezierItem::set_bezier);
}

QRectF graphics::BezierItem::boundingRect() const
{
    return bezier_.bounding_box();
}

void graphics::BezierItem::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget*)
{
    QPainterPath path;
    bezier_.add_to_painter_path(path);
    QPen pen(option->palette.highlight(), 1);
    pen.setCosmetic(true);
    painter->setPen(pen);
    painter->drawPath(path);
}

void graphics::BezierItem::set_type(int index, math::BezierPointType type)
{
    items[index]->set_point_type(type);
}

void graphics::BezierItem::set_bezier(const math::Bezier& bez)
{
    if ( updating )
        return;

    int old_size = bezier_.size();
    bezier_ = bez;

    if ( old_size > bezier_.size() )
        items.erase(items.begin() + bezier_.size(), items.end());

    for ( int i = 0; i < int(items.size()); i++ )
    {
        items[i]->set_index(i);
        items[i]->set_point(bezier_[i]);
    }

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

void graphics::BezierItem::remove_point(int index)
{
    bezier_.points().erase(bezier_.begin() + index);

    for ( int i = index + 1; i < int(items.size()); i++ )
        items[i]->set_index(i-1);

    items.erase(items.begin() + index);
    if ( !bezier_.closed() )
        items.back()->show_tan_out(false);

    do_update(true, tr("Remove Point"));
}

void graphics::BezierItem::do_update(bool commit, const QString& name)
{
    auto lock = updating.get_lock();

    node->push_command(new command::SetMultipleAnimated(
        name,
        commit,
        {&node->shape},
        QVariant::fromValue(bezier_)
    ));

    prepareGeometryChange();
}

void graphics::BezierItem::on_dragged(int index, const math::BezierPoint& point, bool commit, const QString& name)
{
    bezier_.set_point(index, point);
    do_update(commit, name.isEmpty() ? tr("Update shape") : name);
}

void graphics::BezierItem::do_add_point(int index)
{
    items.push_back(std::make_unique<BezierPointItem>(index, bezier_[index], this));
    connect(items.back().get(), &BezierPointItem::modified, this, &BezierItem::on_dragged);
    if ( !bezier_.closed() && index == int(items.size()) - 1 && items.size() > 2)
        items[index-1]->show_tan_out(true);
}

