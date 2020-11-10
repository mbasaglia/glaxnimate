#include "document_node.hpp"
#include "document.hpp"

#include <QPainter>
#include <QGraphicsItem>


model::DocumentNode* model::DocumentNode::docnode_group_parent() const
{
    return nullptr;
}
int model::DocumentNode::docnode_group_child_count() const
{
    return 0;
}
model::DocumentNode* model::DocumentNode::docnode_group_child(int) const
{
    return nullptr;
}
model::DocumentNode* model::DocumentNode::docnode_fuzzy_parent() const
{
    if ( auto p = docnode_group_parent() )
        return p;
    return docnode_parent();
}

QColor model::DocumentNode::docnode_group_color() const
{
    if ( !docnode_valid_color() )
    {
        if ( auto parent = docnode_fuzzy_parent() )
            return parent->docnode_group_color();
        else
            return Qt::white;
    }
    return group_color.get();
}

void model::DocumentNode::on_group_color_changed(const QColor&)
{
    if ( !group_icon.isNull() )
    {
        if ( docnode_valid_color() )
            group_icon.fill(group_color.get());
        else
            group_icon.fill(Qt::white);
    }
    docnode_on_update_group(true);
}

bool model::DocumentNode::docnode_is_instance(const QString& type_name) const
{
    if ( type_name.isEmpty() )
        return true;

    for ( const QMetaObject* meta = metaObject(); meta; meta = meta->superClass() )
    {
        if ( detail::naked_type_name(meta->className()) == type_name )
            return true;
    }

    return false;
}

void model::DocumentNode::docnode_on_update_group(bool force)
{
    if ( force || docnode_valid_color() )
    {
        emit docnode_group_color_changed(this->group_color.get());
        for ( const auto& gc : docnode_group_children() )
            gc->docnode_on_update_group();
        for ( const auto& gc : docnode_children() )
            gc->docnode_on_update_group();
    }
    emit group_transform_matrix_changed(group_transform_matrix(time()));
}

bool model::DocumentNode::docnode_valid_color() const
{
    QColor col = group_color.get();
    return col.isValid() && col.alpha() > 0;
}

QIcon model::DocumentNode::reftarget_icon() const
{
    if ( !docnode_valid_color() )
    {
        if ( auto parent = docnode_fuzzy_parent() )
            return parent->reftarget_icon();
    }

    if ( group_icon.isNull() )
    {
        group_icon = QPixmap{32, 32};
        group_icon.fill(docnode_group_color());
    }

    return group_icon;
}

bool model::DocumentNode::docnode_locked_recursive() const
{
    for ( const DocumentNode* n = this; n; n = n->docnode_parent() )
    {
        if ( n->locked.get() )
            return true;
    }

    return false;
}

void model::DocumentNode::paint(QPainter* painter, FrameTime time, PaintMode mode) const
{
    painter->save();
    if ( mode != NoTransform )
        painter->setTransform(local_transform_matrix(time), true);

    on_paint(painter, time, mode);
    if ( mode >= Recursive )
        for ( const auto& c : docnode_children() )
            c->paint(painter, time, mode);
    painter->restore();
}

bool model::DocumentNode::docnode_selectable() const
{
    if ( !visible.get() || locked.get() )
        return false;
    auto p = docnode_parent();
    if ( p )
        return p->docnode_selectable();
    return true;
}

bool model::DocumentNode::docnode_visible_recursive() const
{
    if ( !visible.get() )
        return false;
    auto p = docnode_parent();
    if ( p )
        return p->docnode_visible_recursive();
    return true;
}

QTransform model::DocumentNode::transform_matrix(model::FrameTime t) const
{
    auto parent = docnode_fuzzy_parent();
    if ( parent )
        return local_transform_matrix(t) * parent->transform_matrix(t);
    return local_transform_matrix(t);
}

QTransform model::DocumentNode::group_transform_matrix(model::FrameTime t) const
{
    auto parent = docnode_group_parent();
    if ( parent )
        return local_transform_matrix(t) * parent->transform_matrix(t);
    return local_transform_matrix(t);
}

void model::DocumentNode::recursive_rename()
{
    document()->set_best_name(this, name.get());
    for ( auto child : docnode_children() )
        child->recursive_rename();
}

void model::DocumentNode::on_visible_changed(bool visible)
{
    emit docnode_visible_changed(visible);
    emit docnode_visible_recursive_changed(visible);

    for ( auto ch : docnode_children() )
        ch->propagate_visible(visible);
}

void model::DocumentNode::propagate_visible(bool visible)
{
    if ( !this->visible.get() )
        return;
    emit docnode_visible_recursive_changed(visible);
    for ( auto ch : docnode_children() )
        ch->propagate_visible(visible && this->visible.get());
}

void model::DocumentNode::propagate_transform_matrix_changed(const QTransform& t_global, const QTransform& t_group)
{
    emit transform_matrix_changed(t_global);
    emit group_transform_matrix_changed(t_group);

    for ( auto ch : docnode_group_children() )
    {
        auto ltm = ch->local_transform_matrix(ch->time());
        ch->propagate_transform_matrix_changed(ltm * t_global, ltm * t_group);
    }

    for ( auto ch : docnode_children() )
    {
        auto ltm = ch->local_transform_matrix(ch->time());
        ch->propagate_transform_matrix_changed(ltm * t_global, ltm);
    }
}

