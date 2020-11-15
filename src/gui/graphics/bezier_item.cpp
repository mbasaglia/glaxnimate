#include "bezier_item.hpp"

#include "math/geom.hpp"

#include "command/animation_commands.hpp"

graphics::PointItem::PointItem(int index, const math::bezier::Point& point, QGraphicsItem* parent, model::AnimatedProperty<math::bezier::Bezier>* property)
: QGraphicsObject(parent), index_(index), point_(point)
{
    connect(&tan_in, &MoveHandle::dragged, this, &PointItem::tan_in_dragged);
    connect(&tan_in, &MoveHandle::drag_finished, this, &PointItem::on_commit);
    connect(&tan_out, &MoveHandle::dragged, this, &PointItem::tan_out_dragged);
    connect(&tan_out, &MoveHandle::drag_finished, this, &PointItem::on_commit);
    connect(&pos, &MoveHandle::dragged, this, &PointItem::pos_dragged);
    connect(&pos, &MoveHandle::drag_finished, this, &PointItem::on_commit);

    pos.setParentItem(this);
    tan_in.setParentItem(this);
    tan_out.setParentItem(this);

    pos.set_role(MoveHandle::Vertex);
    tan_in.set_role(MoveHandle::Tangent);
    tan_out.set_role(MoveHandle::Tangent);

    pos.set_associated_property(property);
    tan_in.set_associated_property(property);
    tan_out.set_associated_property(property);

    set_point(point);
}

const math::bezier::Point & graphics::PointItem::point() const
{
    return point_;
}

void graphics::PointItem::modify(const math::bezier::Point& pt, const QString& undo_name)
{
    set_point(pt);
    on_modified(true, undo_name);
}


QRectF graphics::PointItem::boundingRect() const
{
    return QRectF(tan_in.pos(), tan_out.pos()).normalized();
}

void graphics::PointItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
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

void graphics::PointItem::set_point(const math::bezier::Point& p)
{
    point_ = p;
    pos.setPos(p.pos);
    tan_in.setPos(p.tan_in);
    tan_out.setPos(p.tan_out);

    tan_in.setVisible(has_tan_in && !tan_in_empty());
    tan_out.setVisible(has_tan_out && !tan_out_empty());

    switch ( point_.type )
    {
        case math::bezier::PointType::Corner:
            pos.change_shape(MoveHandle::Diamond, 8);
            break;
        case math::bezier::PointType::Smooth:
            pos.change_shape(MoveHandle::Square, 6);
            break;
        case math::bezier::PointType::Symmetrical:
            pos.change_shape(MoveHandle::Circle, 7);
            break;
    }
}

void graphics::PointItem::set_index(int index)
{
    index_ = index;
}

void graphics::PointItem::set_point_type(math::bezier::PointType type)
{
    point_.type = type;
    point_.adjust_handles_from_type();
    set_point(point_);
    on_modified(true);
}

void graphics::PointItem::show_tan_in(bool show)
{
    tan_in.setVisible(show && has_tan_in);
    update();
}

void graphics::PointItem::show_tan_out(bool show)
{
    tan_out.setVisible(show && has_tan_out);
    update();
}
void graphics::PointItem::tan_in_dragged(const QPointF& p, Qt::KeyboardModifiers mods)
{
    if ( point_.type == math::bezier::Corner && (mods & Qt::ShiftModifier) )
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

void graphics::PointItem::tan_out_dragged(const QPointF& p, Qt::KeyboardModifiers mods)
{
    if ( point_.type == math::bezier::Corner && (mods & Qt::ShiftModifier) )
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

void graphics::PointItem::pos_dragged(const QPointF& p)
{
    auto delta = p - point_.pos;
    tan_in.setPos(point_.tan_in += delta);
    tan_out.setPos(point_.tan_out += delta);
    point_.pos = p;
    on_modified(false);
}

void graphics::PointItem::on_modified(bool commit, const QString& name)
{
    emit modified(index_, point_, commit, name);
}

void graphics::PointItem::on_commit()
{
    on_modified(true);
}

void graphics::PointItem::drag_preserve_angle(QPointF& dragged, QPointF& other, const QPointF& dragged_new)
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

graphics::BezierItem * graphics::PointItem::parent_editor() const
{
    return static_cast<graphics::BezierItem *>(parentItem());
}

int graphics::PointItem::index() const
{
    return index_;
}

void graphics::PointItem::remove_tangent(graphics::MoveHandle* handle)
{
    if ( point_.type == math::bezier::Symmetrical )
        return;

    if ( handle == &tan_in )
        tan_in.setPos(point_.tan_in = point_.pos);
    else if ( handle == &tan_out )
        tan_out.setPos(point_.tan_out = point_.pos);
    else
        return;

    handle->setVisible(false);
    on_modified(true, tr("Remove node tangent"));
    update();
}

bool graphics::PointItem::tan_in_empty() const
{
    return point_.tan_in == point_.pos;
}

bool graphics::PointItem::tan_out_empty() const
{
    return point_.tan_out == point_.pos;
}

void graphics::PointItem::set_has_tan_in(bool show)
{
    has_tan_in = show;
    show_tan_in(!tan_in_empty());
}

void graphics::PointItem::set_has_tan_out(bool show)
{
    has_tan_out = show;
    show_tan_out(!tan_out_empty());
}



graphics::BezierItem::BezierItem(model::Path* node, QGraphicsItem* parent)
: Ctor(parent), node(node)
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

void graphics::BezierItem::set_type(int index, math::bezier::PointType type)
{
    items[index]->set_point_type(type);
}

void graphics::BezierItem::set_bezier(const math::bezier::Bezier& bez)
{
    if ( updating )
        return;

    int old_size = bezier_.size();
    bezier_ = bez;

    // Bezier has fewer points, remove excess
    if ( old_size > bezier_.size() )
    {
        for ( int i = bezier_.size(); i < int(items.size()); i++ )
            selected_indices_.erase(i);
        items.erase(items.begin() + bezier_.size(), items.end());
    }

    // Update points
    for ( int i = 0; i < int(items.size()); i++ )
    {
        items[i]->set_index(i);
        items[i]->set_point(bezier_[i]);
        items[i]->pos.setSelected(selected_indices_.count(i));
    }

    // Bezier has more points, add missing
    if ( old_size < bezier_.size() )
    {
        // Last point might not have the out tangent visible, so ensure it is
        if ( !items.empty() )
            items.back()->set_has_tan_out(true);

        for ( int i = old_size; i < bezier_.size(); i++ )
            do_add_point(i);
    }

    // Hide/show first/last tangent if needed
    if ( !bezier_.empty() )
    {
        items.front()->set_has_tan_in(bezier_.closed());
        items.back()->set_has_tan_out(bezier_.closed());
    }

    prepareGeometryChange();
}

void graphics::BezierItem::remove_point(int index)
{
    bezier_.points().erase(bezier_.begin() + index);

    for ( int i = index + 1; i < int(items.size()); i++ )
        items[i]->set_index(i-1);

    selected_indices_.erase(index);
    items.erase(items.begin() + index);
    if ( !bezier_.closed() )
        items.back()->set_has_tan_out(false);


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

void graphics::BezierItem::on_dragged(int index, const math::bezier::Point& point, bool commit, const QString& name)
{
    bezier_.set_point(index, point);
    do_update(commit, name.isEmpty() ? tr("Update shape") : name);
}

void graphics::BezierItem::do_add_point(int index)
{
    items.push_back(std::make_unique<PointItem>(index, bezier_[index], this, target_property()));
    connect(items.back().get(), &PointItem::modified, this, &BezierItem::on_dragged);
    if ( !bezier_.closed() && index == int(items.size()) - 1 && items.size() > 2)
        items[index-1]->set_has_tan_out(true);
}

model::AnimatedProperty<math::bezier::Bezier> * graphics::BezierItem::target_property() const
{
    return &node->shape;
}

model::DocumentNode* graphics::BezierItem::target_object() const
{
    return node;
}

const std::set<int> & graphics::BezierItem::selected_indices()
{
    return selected_indices_;
}

void graphics::BezierItem::clear_selected_indices()
{
    for ( const auto& item : items )
        item->pos.setSelected(false);
    selected_indices_.clear();
}

void graphics::BezierItem::select_index(int i)
{
    if ( !selected_indices_.count(i) && i >= 0 && i < int(items.size()) )
    {
        selected_indices_.insert(i);
        items[i]->pos.setSelected(true);
    }
}

void graphics::BezierItem::deselect_index(int i)
{
    if ( selected_indices_.count(i) )
    {
        selected_indices_.erase(i);
        if ( i >= 0 && i < int(items.size()) )
            items[i]->pos.setSelected(false);
    }
}

void graphics::BezierItem::toggle_index(int i)
{
    if ( selected_indices_.count(i) )
        deselect_index(i);
    else
        select_index(i);
}

const math::bezier::Bezier & graphics::BezierItem::bezier() const
{
    return bezier_;
}
